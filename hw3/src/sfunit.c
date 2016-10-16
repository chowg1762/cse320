#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/sfmm.h"



/***********DIRECTORY************/
 //c_sfunit.c starts at line 74
 //y_sfunit.c starts at line 536
 //s_sfunit.c starts at line 925
 //j_sfunit.c starts at line 1135
/***********DIRECTORY************/


/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == x - 8);
    sf_free_header *headofx = (sf_free_header*) (x - 8);
    sf_footer *footofx = (sf_footer*) (x - 8 + (headofx->header.block_size << 4)) - 8;

    sf_blockprint((sf_free_header*)((void*)x - 8));
    // All of the below should be true if there was no coalescing
    cr_assert(headofx->header.alloc == 0);
    cr_assert(headofx->header.block_size << 4 == 32);
    cr_assert(headofx->header.padding_size == 0);

    cr_assert(footofx->alloc == 0);
    cr_assert(footofx->block_size << 4 == 32);
}

/*
//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
*/

Test(sf_memsuite, Coalesce_up, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    void *z = sf_malloc(4);
    memset(x, 1, 4);
    memset(y, 1, 4);
    memset(z, 1, 4);

    sf_free(y);
    sf_free_header *y_header = y - 8;
    sf_footer *y_footer = y + 16;
    cr_assert(freelist_head == y_header);
    cr_assert(y_header->header.alloc == 0);
    cr_assert(y_header->header.block_size << 4 == 32);
    cr_assert(y_footer->alloc == y_header->header.alloc);
    cr_assert(y_footer->block_size == y_header->header.block_size);

    sf_free(x);

    //sf_varprint(x);
    //sf_varprint(y);
    sf_free_header *x_header = x - 8;
    sf_footer *x_footer = ((void*)x_header) +(x_header->header.block_size << 4) - 8; 
    cr_assert(x_footer == y_footer);
    cr_assert(x_header == freelist_head);
    cr_assert(x_header->header.block_size << 4 == 32 + 32);
}

Test(sf_memsuite, Coalesce_down, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    void *z = sf_malloc(4);
    memset(x, 1, 4);
    memset(y, 1, 4);
    memset(z, 1, 4);

    sf_free(x);
    sf_free_header *x_header = x - 8;
    sf_footer *x_footer = x + 16;
    cr_assert(freelist_head == x_header);
    cr_assert(x_header->header.alloc == 0);
    cr_assert(x_header->header.block_size << 4 == 32);
    cr_assert(x_footer->alloc == x_header->header.alloc);
    cr_assert(x_footer->block_size == x_header->header.block_size);

    sf_free(y);
    sf_free_header *y_header = x - 8;
    cr_assert(y_header == freelist_head);
    cr_assert(y_header->header.block_size << 4 == 32 + 32);
}

Test(sf_memsuite, Coalesce_up_down, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    void *z = sf_malloc(4);
    void *a = sf_malloc(4);
    memset(x, 1, 4);
    memset(y, 2, 4);
    memset(z, 3, 4);
    memset(a, 4, 4);

    sf_free(x);
    sf_free_header *x_header = x - 8;
    cr_assert(x_header == freelist_head);
    cr_assert(x_header->header.block_size << 4 == 32);

    sf_free(z);
    sf_free_header *z_header = z - 8;
    cr_assert(freelist_head == z_header);

    sf_free(y);
    sf_free_header *y_header = y - 8;
    cr_assert(freelist_head != y_header);
    cr_assert(freelist_head == x_header);
    cr_assert(y_header->header.alloc == 0);
    cr_assert(x_header->header.block_size << 4 == 32 * 3);
}

Test(sf_memsuite, Malloc_large_space, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(8176); // 2 pages
    void *y = sf_malloc(32); // on to 3rd page
    memset(x, 2, 8176);
    memset(y, 3, 32);

    //sf_varprint(x);
    sf_header *x_header = x - 8, *y_header = y - 8;
    sf_footer *x_footer = (void*)x_header + (x_header->block_size << 4) - 8;
    cr_assert(x_header->alloc == 1);
    cr_assert(x_header->block_size << 4 == 8192);
    cr_assert(x_header->block_size == x_footer->block_size);
    cr_assert(((void*)x_header) + (x_header->block_size << 4) == y_header);

    sf_free(x);
    cr_assert(freelist_head == (sf_free_header*)x_header);
}

Test(sf_memsuite, Splinter_at_page_end, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4060);
    memset(x, 4, 4060);

    sf_header* x_header = x - 8;
    sf_footer *x_footer = (void*)x_header + (x_header->block_size << 4) - 8;
    cr_assert(x_header->block_size << 4 == 4096);
    cr_assert(x_header->block_size == x_footer->block_size);
    cr_assert(x_header->padding_size == 4);
}

Test(sf_memsuite, Splinter_from_realloc_shrink, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(32);
    void *y = sf_malloc(10);
    memset(x, 3, 32);
    memset(y, 4, 10);

    sf_header *x_header = x - 8;
    size_t x_old_size = x_header->block_size;

    x = sf_realloc(x, 12);
    //sf_varprint(x);
    cr_assert(x_header->block_size == x_old_size);
    cr_assert(x_header->padding_size == 4);
}

Test(sf_memsuite, Splinter_from_realloc_expand, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(32);
    void *y = sf_malloc(32);
    void *z = sf_malloc(32);
    memset(x, 3, 32);
    memset(y, 4, 32);
    memset(z, 5, 32);

    sf_header *x_header = x - 8;
    size_t x_old_size = x_header->block_size;
    sf_varprint(x);

    sf_free(y);
    x = sf_realloc(x, 55);
    //sf_varprint(x);
    cr_assert(x_header->block_size != x_old_size);
    cr_assert(x_header->block_size << 4 == 48 * 2);
    cr_assert(x_header->padding_size == 9);
    cr_assert(freelist_head != y - 8);
}

Test(sf_memsuite, Free_block_from_realloc_shrink, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(48);
    void *y = sf_malloc(1);
    memset(x, 3, 48);
    memset(y, 9, 1);

    sf_realloc(x, 14);
    cr_assert(freelist_head == x + 24);
    sf_blockprint(freelist_head);
    cr_assert(freelist_head->header.block_size << 4 == 32);
    cr_assert(freelist_head->header.padding_size == 0);
}

Test(sf_memsuite, Realloc_find_new_area, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(16);
    void *y = sf_malloc(32);
    memset(x, 15, 16);
    memset(y, 14, 32);

    void *new_x = sf_realloc(x, 32);
    sf_free_header *x_header = x - 8, *new_x_header = new_x - 8;

    cr_assert(new_x != x);
    cr_assert(x_header->header.alloc == 0);
    cr_assert(new_x_header->header.block_size << 4 == 48);
    cr_assert(freelist_head == x_header);
}

Test(sf_memsuite, Bad_requests, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(0);
    cr_assert(x == NULL);

    x = sf_malloc(20000);
    cr_assert(x == NULL); 

    x = sf_realloc(x, 0);
    cr_assert(x == NULL);

    x = (void*)2;
    x = sf_realloc(x, 15);
    cr_assert(x == NULL);
}

Test(sf_memsuite, Realloc_repeated_requests, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(16);
    sf_header *x_header = x - 8, *x_new_header;
    x = sf_realloc(x, 4);
    x_new_header = x - 8;
    cr_assert(x_header->block_size << 4 == 32);
    cr_assert(x_header->padding_size == 12);
    cr_assert(x_header == x_new_header);

    void *y = sf_malloc(10);
    x = sf_realloc(x, 2000);
    x_new_header = x - 8;
    cr_assert(x_new_header->block_size << 4 == 2016);
    cr_assert(x_new_header->padding_size == 0);
    cr_assert(x_header != x_new_header);
    cr_assert(freelist_head == (sf_free_header*)x_header);

    sf_malloc(10); // after x
    sf_free(y);

    x = sf_realloc(x, 2010); // needs y's space
    x_new_header = x - 8;
    cr_assert(x_new_header->block_size << 4 == 2032);
    cr_assert(x_new_header->padding_size == 6);
}

Test(sf_memsuite, Info_test, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(10); //i + 22 | a + 1
    void *a = sf_malloc(12); //i + 20 | a + 1
    void *y = sf_malloc(160); // i + 16 | a + 1
    void *z = sf_malloc(168); // i + 24 | a + 1
    memset(x, 2, 1);

    sf_free(a); // i - 20 | f + 1
    sf_free(y); // i - 16 | c + 1 | f + 1
    sf_free(z); // i - 24 | c + 1 | f + 1

    info mem;
    info *meminfo = &mem;
    cr_assert(sf_info(meminfo) == 0);
    cr_assert(meminfo->internal == 22);
    cr_assert(meminfo->allocations == 4);
    cr_assert(meminfo->coalesce == 2);
    cr_assert(meminfo->external == 4064);
    cr_assert(meminfo->frees == 3);

    cr_assert(sf_info(NULL) == -1);
}

Test(sf_memsuite, Malloc_after_crashed_malloc, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(20000);
    cr_assert(x == NULL);
    
    x = sf_malloc(10);
    sf_header* x_header = x - 8;
    cr_assert(x_header->block_size == 32);
    cr_assert(x_header->padding_size == 6);
    cr_assert(freelist_head != NULL);
}

