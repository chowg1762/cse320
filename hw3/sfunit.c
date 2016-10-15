#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
    printf("SUCESS!!! Malloc_an_Integer Passed!!!\n");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
    printf("SUCESS!!! Free_block_check_header_footer_values Passed!!!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
    printf("SUCESS!!! PaddingSize_Check_char Passed!!!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
    printf("SUCESS!!! Check_next_prev_pointers_of_free_block_at_head_of_list Passed!!!\n");
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4);
    void *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == x-8);
    sf_free_header *headofx = (sf_free_header*) (x-8);
    sf_footer *footofx = (sf_footer*) (x - 8 + (headofx->header.block_size << 4)) - 8;

    sf_blockprint((sf_free_header*)((void*)x-8));
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
Test(sf_memsuite, Address_Word_Aligned, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(sizeof(int));
    cr_assert(((uint64_t)x % sizeof(long double)) == 0, "Memory address not aligned on word boundary!");
    printf("SUCESS!!! Address_Word_Aligned Passed!!!\n");
}

Test(sf_memsuite, Splinter_Check, .init = sf_mem_init, .fini = sf_mem_fini) {
    long unsigned int alloc_size = 4096-46;
    void *x = sf_malloc(alloc_size);
    x -= SF_HEADER_SIZE;
    sf_header* head = (sf_header*)x;
    cr_assert(head->alloc == 1, "Alloc bit in header is not 1!");
    cr_assert(head->block_size<<4 == 4096, "Splinter not incorporated into block!");
    cr_assert(head->padding_size == 14, "Incorrect padding!");
    printf("SUCESS!!! Splinter_Check Passed!!!\n");
}

Test(sf_memsuite, Coalesce_Down_Check, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4), *y = sf_malloc(4), *z = sf_malloc(4);
    sf_header* headx = (sf_header*)(x - SF_HEADER_SIZE);
    sf_free(y);
    sf_free(x);
    cr_assert(((sf_header*)((char*)z-SF_HEADER_SIZE))->alloc == 1, "Error allocating memory!");
    cr_assert(headx->alloc == 0, "Alloc bit in header is not 0!");
    cr_assert(headx->block_size<<4 == 64);
    printf("SUCESS!!! Coalesce_Down_Check Passed!!!\n");
}

Test(sf_memsuite, Coalesce_Up_Check, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4), *y = sf_malloc(4), *z = sf_malloc(4);
    sf_header* headx = (sf_header*)(x - SF_HEADER_SIZE);
    sf_free(x);
    sf_varprint(x);
    sf_free(y);
    sf_varprint(x);
    sf_varprint(y);
    cr_assert(((sf_header*)((char*)z-SF_HEADER_SIZE))->alloc == 1, "Error allocating memory!");
    cr_assert(headx->alloc == 0, "Alloc bit in header is not 0!");
    cr_assert(headx->block_size<<4 == 64);
    printf("SUCESS!!! Coalesce_Up_Check Passed!!!\n");
}

Test(sf_memsuite, Coalesce_UP_AND_DOWN_Check, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4), *y = sf_malloc(4), *z = sf_malloc(4), *t = sf_malloc(4);
    sf_header* headx = (sf_header*)(x - SF_HEADER_SIZE);
    sf_free(x);
    sf_free(z);
    sf_free(y);
    cr_assert(((sf_header*)((char*)t-SF_HEADER_SIZE))->alloc == 1, "Error allocating memory!");
    cr_assert(headx->alloc == 0, "Alloc bit in header is not 0!");
    cr_assert(headx->block_size<<4 == 96);
    printf("SUCESS!!! Coalesce_UP_AND_DOWN_Check Passed!!!\n");
}