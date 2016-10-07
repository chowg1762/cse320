#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sfmm.h"

#define DOUBLE_WORD 8
#define MAX_DATA_SIZE 16
#define PAGE_SIZE 4096

void* start_of_heap;
void* end_of_heap;

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;

/*
struct __attribute__((__packed__)) sf_header {
    uint64_t alloc : ALLOC_SIZE_BITS;
    uint64_t block_size : BLOCK_SIZE_BITS;
    uint64_t unused_bits : UNUSED_SIZE_BITS;
    uint64_t padding_size : PADDING_SIZE_BITS;
}; */

void coalesce(void *ptr) {

}

void *sf_malloc(size_t size){
  // Handle trivial requests
  if (size == 0) {
    return NULL;
  }

  // Search free list for suitable block
  size_t padding = size % MAX_DATA_SIZE; // Handle shards
  size += MAX_DATA_SIZE; // For header and footer
  sf_free_header* cursor = freelist_head, *new_free_header;
  sf_footer* footer, *new_free_footer;
  while (cursor != NULL) {
    if (cursor->header.block_size << 4 >= size) {
      size_t size_difference = cursor->header.block_size - size;
      // Remaining space is large enough for another block
      if (size_difference >= 32) {
        // Make new free block with header and footer
        new_free_header = cursor + size + padding;
        cursor->prev->next = new_free_header;
        cursor->next->prev = new_free_header;
        new_free_header->header.alloc = 0;
        new_free_header->header.block_size = size_difference >> 4;
        new_free_header->header.padding_size = 0;
        new_free_footer = (void*)new_free_header + size_difference;
        new_free_footer->alloc = 0;
        new_free_footer->block_size = size_difference >> 4;
        // Replace cursor with new free block in free list
        new_free_header->prev = cursor->prev;
        new_free_header->next = cursor->next;
      }
      // Set Header
      cursor->header.alloc = 1;
      cursor->header.block_size = size + padding;
      cursor->header.padding_size = padding;
      // Set Footer
      footer = (void*)cursor + cursor->header.block_size - 8;
      footer->alloc = 1;
      footer->block_size = cursor->header.block_size;
      return cursor + DOUBLE_WORD; // Payload
    } else {
      cursor = cursor->next;
    }
  }  

  // Expand the heap since there isn't a suitable block
  if (sf_sbrk(0) == (void*)-1) {
    errno = ENOMEM;
    return NULL;
  }
  void* brk_pos = sf_sbrk(1);
  
  footer = brk_pos - DOUBLE_WORD;
  if (footer->alloc == 0) {
    cursor = ((void*)footer) - footer->block_size + DOUBLE_WORD;
  } else {
    cursor = (sf_free_header*)brk_pos;
  }
  // Set Header 
  cursor->header.alloc = 1;
  cursor->header.block_size = size + padding;
  cursor->header.padding_size = padding;
  // Set Footer
  footer = ((void*)cursor) + (cursor->header.block_size << 4) - DOUBLE_WORD;
  footer->alloc = 1;
  footer->block_size = cursor->header.block_size;

  // Make free block for new heap area
  new_free_header = ((void*)footer) + DOUBLE_WORD;
  new_free_footer = brk_pos + PAGE_SIZE - DOUBLE_WORD;
  new_free_header->header.alloc = 0;
  new_free_header->header.block_size = ((void*)new_free_footer) - ((void*)new_free_header);
  new_free_footer->alloc = 0;
  new_free_footer->block_size = new_free_header->header.block_size;

  // Add new free block as head of freelist
  freelist_head->prev = new_free_header;
  new_free_header->next = freelist_head;
  freelist_head = new_free_header;

  // Return start of payload
  return cursor + DOUBLE_WORD;
}

void sf_free(void *ptr){

}

void *sf_realloc(void *ptr, size_t size){
  return NULL;
}

int sf_info(info* meminfo){
  return -1;
}
