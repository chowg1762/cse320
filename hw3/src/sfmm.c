#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sfmm.h"

#define MAX_DATA_SIZE 16
#define PAGE_SIZE 4096
#define HDRP(ptr) ((sf_header*)ptr)
#define FTRP(ptr) ((sf_footer*)ptr)
#define NEXT_BLKP(ptr) (ptr + (((sf_header*)ptr)->block_size << 4))
#define PREV_BLKP(ptr) (ptr - ((((sf_footer*)(ptr - 8))->block_size) << 4))

void* start_of_heap;
void* end_of_heap;

size_t internal; // header footer padding - bytes 
size_t external; // free blocks - bytes
size_t allocations;
size_t frees;
size_t coalesces;

sf_free_header* freelist_head = NULL;

/*
* Inserts the free block into the freelist as a new head
*
* @param ptr Pointer to the free block to insert
*/
void insert_node(void *ptr) {
  sf_free_header *header = ptr;
  if (freelist_head != NULL)
    freelist_head->prev = header;
  header->next = freelist_head;
  header->prev = NULL;
  freelist_head = header;
}

/*
* Removes the free block from the freelist
*
* @param ptr Pointer to the free block in the freelist
*/
void remove_node(void *ptr) {
  sf_free_header *node = ptr;
  // Node is an allocated block
  if (node->header.alloc == 1)
    return;
  // Node is inside the list
  if (node->prev != NULL && node->next != NULL) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  } 
  // Node is the freelist tail
  else if (node->prev != NULL) {
    node->prev->next = NULL;
  } 
  // Node is the freelist_head - make its next the head
  else if (node->next != NULL) {
    freelist_head = node->next;
    node->next->prev = NULL;
  } 
  // Node is the freelist_head and has no next - remove it
  else {
    if (freelist_head == node)
      freelist_head = NULL;
  }
}

/**
* Writes a block in the heap or just changes alloc status if no block size
* is specified
*
* @param ptr Pointer to the start of the block
* @param alloc Allocation status of the block
* @param padding Padding size of the block's payload
* @param block_size Total size of the block including header, footer, splinter 
*/
void write_block(void *ptr, size_t alloc, size_t block_size, size_t padding) {
  sf_header *header = ptr;
  sf_footer *footer = ptr + block_size - SF_FOOTER_SIZE;
  
  if (block_size == 0) {
    header->alloc = alloc;
    footer->alloc = alloc;
    if (alloc == 0)
      header->padding_size = 0;
    return;
  }

  header->alloc = alloc;
  header->padding_size = padding;
  header->block_size = block_size >> 4;
  footer->alloc = alloc;
  footer->block_size = header->block_size;
}

/*
* Merges adjacent free blocks and returns a pointer to the combined header
*
* @param ptr Pointer to the newly freed block
*/
void *coalesce(void *ptr) {
  // Check if block is free
  sf_header *header = ptr;
  if (header->alloc == 0) {
    external -= header->block_size << 4;
  } else {
    write_block(ptr, 0, 0, 0);
  }

  // Check if neighboring blocks are free
  size_t size = HDRP(ptr)->block_size << 4, 
  prev_alloc = 1, next_alloc = 1;

  // Check if block exists on heap bounds
  if (ptr > start_of_heap) {
    prev_alloc = HDRP(PREV_BLKP(ptr))->alloc;
  }
  if (ptr + size < end_of_heap) {
    next_alloc = HDRP(NEXT_BLKP(ptr))->alloc;
  }

  // [A][T][A] (no coalesce)
  if (prev_alloc && next_alloc) {
    header = ptr;
  }
  
  // [A][T][F] (coalesce up)
  else if (prev_alloc && !next_alloc) {
    header = ptr;
    sf_header *next_header = NEXT_BLKP(ptr);
    external -= next_header->block_size << 4;
    size += next_header->block_size << 4;
    remove_node(next_header);
    ++coalesces;
  } 

  // [F][T][A] (coalesce down)
  else if (!prev_alloc && next_alloc) {
    header = PREV_BLKP(ptr);
    external -= header->block_size << 4;
    size += header->block_size << 4;
    remove_node(header);
    ++coalesces;
  }

  // [F][T][F] (coalesce full)
  else {
    header = PREV_BLKP(ptr);
    sf_header *next_header = NEXT_BLKP(ptr);
    external -= (header->block_size << 4) + (next_header->block_size << 4);
    size += (header->block_size << 4) + (next_header->block_size << 4);
    remove_node(header);
    remove_node(next_header);
    ++coalesces;
  }

  // Add to freelist as head
  write_block(header, 0, size, 0);
  insert_node(header);
  external += size;
  return ptr;
}

/*
* Places a block of new_size in the block located at ptr
*/
void place(void* ptr, size_t new_size, size_t padding) {
  size_t size_difference = (HDRP(ptr)->block_size << 4) - new_size;

  // Split block and make free block
  if (size_difference >= 32) {
    remove_node(ptr);
    write_block(ptr, 1, new_size, padding);
    write_block(NEXT_BLKP(ptr), 0, size_difference, 0);
    coalesce(NEXT_BLKP(ptr));
  }
  // Splinter
  else {
    // Merge splinter with following free block
    if (HDRP(NEXT_BLKP(ptr))->alloc == 0) {
      size_t merged_size = HDRP(NEXT_BLKP(ptr))->block_size + size_difference;
      write_block(ptr, 1, new_size, padding);
      write_block(ptr, 0, merged_size, 0);
    } 
    // Over-allocate to avoid Splinter
    else {
      write_block(ptr, 1, new_size + size_difference, padding);
    }
  }
}

void *find_fit(size_t size) {
  // Search free list for suitable block
  sf_free_header *cursor = freelist_head;
  while (cursor != NULL) {
    if (cursor->header.block_size << 4 >= size) {
      return cursor;
    } else {
      cursor = cursor->next;
    }
  }
  return NULL;
}

void *expand_heap(size_t size) {
  // Initialize
  if (start_of_heap == 0) {
    start_of_heap = end_of_heap = sf_sbrk(0);
  }

  // Find lower bound of new possible block
  void *ptr, *prev_end = end_of_heap;
  sf_footer *footer = end_of_heap - SF_FOOTER_SIZE;
  // If last block is free, include it as lower bound
  if ((void*)footer > start_of_heap && footer->alloc == 0) {
    ptr = end_of_heap - (footer->block_size << 4) + SF_HEADER_SIZE;
  } else {
    ptr = end_of_heap;
  }

  // Expand heap until enough space is made
  while (size > end_of_heap - ptr) {
    if (sf_sbrk(1) == (void*)-1) {
      // Make free block for new heap area
      write_block(ptr, 0, end_of_heap - ptr, 0);
      insert_node(ptr);
      return NULL; 
    }
    end_of_heap = sf_sbrk(0);
  }

  // Heap wasnt able to expand
  if (prev_end == end_of_heap) {
    return NULL;
  }

  // Make free block for new heap area
  write_block(ptr, 1, end_of_heap - ptr, 0);
  coalesce(ptr);

  return ptr;
}

void *sf_malloc(size_t size) {
  // Handle trivial cases
  if (size == 0) {
    return NULL;
  }

  void *ptr;
  size_t padding = (size % MAX_DATA_SIZE == 0)? 0 : 
  MAX_DATA_SIZE - (size % MAX_DATA_SIZE);
  size += padding + SF_HEADER_SIZE + SF_FOOTER_SIZE;

  // Search freelist for a suitable space (first-fit)
  if ((ptr = find_fit(size)) != NULL) {
    place(ptr, size, padding);
    return ptr + SF_HEADER_SIZE;
  }

  // No suitable free block found - Expand heap
  else if ((ptr = expand_heap(size)) == NULL) {
    errno = ENOMEM;
    return NULL; // 605030 to 605070
  }

  // Place new allocated block in
  remove_node(freelist_head); 
  place(ptr, size, padding);

  // Return payload
  ++allocations;
  return ptr + SF_HEADER_SIZE;
}

void sf_free(void *ptr) {
  // Handle trivial cases
  if (ptr == NULL)
    return;
  
  ptr -= SF_HEADER_SIZE;

  // Handle non heap locations
  if (ptr < start_of_heap || ptr > end_of_heap) {
    errno = EINVAL; 
    return;
  } 

  // Handle double free
  sf_header *header = ptr;
  if (header->alloc == 0) {
    errno = EINVAL;
    return;
  }

  coalesce(ptr);
}

void *sf_realloc(void *ptr, size_t size) {
  // Handle bad calls
  if (size == 0 || ptr < start_of_heap || ptr > end_of_heap) {
    errno = EINVAL;
    return NULL;
  }
  
  // Call as malloc
  if (ptr == NULL) {
    return sf_malloc(size);
  }
  
  ptr -= SF_HEADER_SIZE;
  size_t old_size = HDRP(ptr)->block_size << 4,
  padding = (size % MAX_DATA_SIZE == 0)? 0 : 
  MAX_DATA_SIZE - (size % MAX_DATA_SIZE);
  size += padding + SF_HEADER_SIZE + SF_FOOTER_SIZE;

  // Shrink
  if (old_size > size) {
    place(ptr, size, padding);
  } 
  // Expand
  else if (old_size < size) { 
    sf_header *next_header = NEXT_BLKP(ptr);
    // Check if upper adjacent block is free and large enough
    if (next_header->alloc == 0 && 
    (old_size + (next_header->block_size << 4)) >= size) {
      write_block(ptr, 1, old_size + (next_header->block_size << 4), padding);
      remove_node(next_header);
      external -= next_header->block_size << 4;
      place(ptr, size, padding);
    } 
    // Cant extend here, find a new area
    else {
      void *new_ptr = sf_malloc(size);
      if (new_ptr == NULL) {
        return NULL;
      }
      memmove(new_ptr + SF_HEADER_SIZE, ptr + SF_HEADER_SIZE, size - 16);
      free(ptr);
    } 
 } 
  // Same size
  else {
    write_block(ptr, 1, size, padding);
  }

  return ptr + SF_HEADER_SIZE;
}

int sf_info(info* meminfo){
  if (meminfo == NULL) {
    return -1;
  }
  meminfo->internal = internal;
  meminfo->external = external;
  meminfo->allocations = allocations;
  meminfo->frees = frees;
  meminfo->coalesce = coalesces;
  return 0;
}