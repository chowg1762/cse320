#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sfmm.h"

#define MAX_DATA_SIZE 16
#define PAGE_SIZE 4096

#define HDRP(ptr) ((sf_free_header*)(ptr))
#define FTRP(ptr) ((sf_footer*)(ptr + (((sf_header*)ptr)->block_size << 4) - 8))
#define NEXT_BLKP(ptr) (ptr + (((sf_header*)ptr)->block_size << 4))
#define PREV_BLKP(ptr) (ptr - ((((sf_footer*)(ptr - 8))->block_size) << 4))

void* start_of_heap;
void* end_of_heap;

size_t internal; // header footer padding - bytes 
size_t external; // free blocks - bytes
size_t allocations;
size_t frees;
size_t coalesces;

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;

void insert_node(sf_free_header* header) {
  if (freelist_head != NULL)
    freelist_head->prev = header;
  header->next = freelist_head;
  header->prev = NULL;
  freelist_head = header;
}

void remove_node(void *ptr) {
  sf_free_header *node = ptr;
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

void *coalesce(void *ptr) {
  // Check if neighboring blocks are free
  size_t size = HDRP(ptr)->header.block_size << 4, 
  prev_alloc = 1, next_alloc = 1;
  sf_free_header *header;
  sf_footer *footer;

  if (ptr > start_of_heap) {
    prev_alloc = HDRP(PREV_BLKP(ptr))->header.alloc;
  }
  if (size + ptr < end_of_heap) {
    next_alloc = HDRP(NEXT_BLKP(ptr))->header.alloc;
  }

  header = HDRP(ptr);
  header->header.alloc = 0;
  header->header.padding_size = 0;

  // [A][T][A] - Add to freelist as head
  if (prev_alloc && next_alloc) {
    footer = FTRP(ptr);
    footer->alloc = 0;
    --coalesces;
  }
  
  // [A][T][F]
  else if (prev_alloc && !next_alloc) {
    sf_free_header *next_header = HDRP(NEXT_BLKP(ptr));
    footer = FTRP((void*)next_header);
    header->header.block_size += next_header->header.block_size;
    footer->block_size = header->header.block_size;
    remove_node(next_header);
  } 

  // [F][T][A]
  else if (!prev_alloc && next_alloc) {
    header = HDRP(PREV_BLKP(ptr));
    footer = FTRP(ptr);
    header->header.block_size += (size >> 4);
    footer->alloc = 0;
    footer->block_size = header->header.block_size;
    remove_node(header);
  }

  // [F][T][F]
  else {
    sf_free_header *next_header = HDRP(NEXT_BLKP(ptr));
    header = HDRP(PREV_BLKP(ptr));
    footer = FTRP(NEXT_BLKP(ptr));
    header->header.block_size += footer->block_size + (size >> 4);
    footer->block_size = header->header.block_size;
    remove_node(header);
    remove_node(next_header);
  }

  // Add to freelist as head
  insert_node(header);
  ++coalesces;
  external += size;

  return ptr;
}

void split_block(void* ptr, size_t new_size, size_t padding) {
  sf_free_header* header = ptr;
  sf_footer* footer;
  size_t full_size = header->header.block_size << 4;
  size_t size_difference = full_size - new_size;

  if (((sf_header*)ptr)->alloc == 0) {
    remove_node(ptr);
    external -= full_size;
  } else {
    internal -= header->header.padding_size;
  }
  internal += padding + SF_FOOTER_SIZE + SF_HEADER_SIZE;

  // Split block
  if (size_difference >= 16) {
    // Make allocated block
    header->header.alloc = 1;
    header->header.block_size = new_size >> 4;
    header->header.padding_size = padding;
    footer = ptr + new_size - SF_FOOTER_SIZE;
    footer->alloc = 1;
    footer->block_size = header->header.block_size;
  }

  // Make free block
  if (size_difference >= 32) {
    header = ptr + new_size;
    header->header.alloc = 0;
    header->header.block_size = size_difference >> 4;
    header->header.padding_size = 0;
    footer = ptr + full_size - SF_FOOTER_SIZE;
    footer->alloc = 0;
    footer->block_size = header->header.block_size;
    coalesce(header);
  } 
  // Splinter
  else {
    // Merge splinter with neighboring free-block
    if (size_difference == 16 && (ptr + full_size) < end_of_heap && 
    HDRP(ptr + full_size)->header.alloc == 0) {
      header = ptr + new_size;
      header->header.alloc = 0;
      header->header.block_size = (SF_HEADER_SIZE + SF_FOOTER_SIZE + 
      (HDRP(ptr + full_size)->header.block_size << 4)) >> 4;
      footer = ((void*)header) + (header->header.block_size << 4) - SF_FOOTER_SIZE;
      footer->block_size = header->header.block_size;
      remove_node(ptr + full_size);
      insert_node(header);
      external += size_difference;
    } 
    // Over-allocate
    else {
      header->header.alloc = 1;
      header->header.block_size = full_size >> 4;
      footer = ptr + full_size - SF_FOOTER_SIZE;
      footer->alloc = 1;
      footer->block_size = header->header.block_size;
    }
  }
}

void *find_fit(size_t size, size_t padding) {
  // Search free list for suitable block
  sf_free_header *cursor = freelist_head;
 // sf_footer *footer;
  while (cursor != NULL) {
    if (cursor->header.block_size << 4 >= size) {
      split_block(cursor, size, padding);
      return cursor; // Header
    } else {
      cursor = cursor->next;
    }
  }
  return NULL;
}

void *sf_malloc(size_t size) {
  // Handle trivial cases
  if (size == 0) {
    return NULL;
  }

  void *ptr;
  size_t padding = (size % MAX_DATA_SIZE > 0)? // Handle quad word allignment
    (MAX_DATA_SIZE - size % MAX_DATA_SIZE) : 0;
  size += padding + MAX_DATA_SIZE; // Add size of padding, header, and footer
  
  // Search freelist for a suitable block
  if ((ptr = find_fit(size, padding)) != NULL) {
    ++allocations;
    return ptr + SF_HEADER_SIZE; // Payload
  } 

  if (!start_of_heap) {
    start_of_heap = end_of_heap = sf_sbrk(0);
    ptr = start_of_heap;
  } else {
    ptr = end_of_heap;
  }

  sf_free_header *header;
  sf_footer *footer;

  footer = ptr - SF_FOOTER_SIZE;
  if (ptr > start_of_heap && ptr <= end_of_heap &&  footer->alloc == 0) {
    header = ((void*)footer) - (footer->block_size << 4) + SF_HEADER_SIZE;
  } else {
    header = (sf_free_header*)ptr;
  }
  
  // Expand the heap since there isn't a suitable block
  while (end_of_heap - (void*)header < size) { 
    end_of_heap = sf_sbrk(0);
    if (sf_sbrk(1) == (void*)-1) {
      // Make new heap space a free block
      if (footer->alloc == 0) {
        remove_node(header);
      }
      header->header.alloc = 0;
      header->header.block_size = (end_of_heap - (void*)header) >> 4;
      footer = (void*)header + (header->header.block_size << 4) - SF_FOOTER_SIZE;
      footer->alloc = 0;
      footer->block_size = header->header.block_size;
      insert_node(header);

      errno = ENOMEM;
      return NULL;
    }
  } 
  
  // Set Header
  header->header.alloc = 1;
  header->header.block_size = (end_of_heap - ((void*)header)) >> 4;

  split_block(header, size, padding);

  ++allocations;

  // Return start of payload
  return ((void*)header) + SF_HEADER_SIZE;
}

void sf_free(void *ptr) {
  // Handle trivial cases
  if (ptr == NULL)
    return;
  
  if (ptr < start_of_heap || ptr > end_of_heap) {
    errno = EINVAL; 
    return;
  }

  ptr -= SF_HEADER_SIZE;
  if (!((sf_header*)ptr)->alloc)
    return;

  ++frees;
  internal -= HDRP(ptr)->header.padding_size + SF_HEADER_SIZE + SF_FOOTER_SIZE;  

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
  sf_free_header *header = ptr;
  sf_footer *footer;
  size_t padding = (size % MAX_DATA_SIZE > 0)? 
    (MAX_DATA_SIZE - size % MAX_DATA_SIZE) : 0;
  size_t old_size = header->header.block_size << 4;
  size += padding + SF_HEADER_SIZE + SF_FOOTER_SIZE;

  // Expand
  if (size > old_size) {
    // Followed by a large enough free block
    if (HDRP(NEXT_BLKP(ptr))->header.alloc == 0 &&
    size <= old_size + (HDRP(NEXT_BLKP(ptr))->header.block_size << 4)) {
      // Split block and coalesce or merge
      header->header.block_size = (old_size >> 4) + 
        HDRP(NEXT_BLKP(ptr))->header.block_size;
      footer = ptr + header->header.block_size;
      footer->alloc = 1;
      footer->block_size = header->header.block_size;
      split_block(ptr, size, padding);
    } 
    // No: find another free block from malloc, memcopy, free
    else {
      void *new_ptr = sf_malloc(size - padding - SF_HEADER_SIZE - SF_FOOTER_SIZE);
      memmove(new_ptr, (ptr + SF_HEADER_SIZE), (old_size - SF_HEADER_SIZE));
      sf_free(ptr + SF_HEADER_SIZE);
      return new_ptr;
    }  
  } 
  // Shrink
  else if (size < old_size) {
    split_block(ptr, size, padding);
  } 
  // Same size
  else {
    internal -= header->header.padding_size;
    internal += padding;
    header->header.padding_size = padding;
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
