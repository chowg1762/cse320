#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../include/sfmm.h"



/***********DIRECTORY************/
 //c_sfunit.c starts at line 74
 //y_sfunit.c starts at line 536
 //s_sfunit.c starts at line 925
 //j_sfunit.c starts at line 1135
 //e_sfunit.c starts at line 1321
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

//c_sfunit.c

//1
Test(sf_memsuite, free_in_order, .init = sf_mem_init, .fini = sf_mem_fini) {
    //allocating some things and then freeing sequentially in order
    sf_malloc(4);
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    sf_malloc(4);

    sf_free(a);
    sf_free(b);
    sf_free(c);

    cr_assert(freelist_head == a - 8);
    cr_assert(freelist_head->header.alloc == 0);
    cr_assert(freelist_head->header.block_size << 4 == 96, "Expected 96, was given: %d.", freelist_head->header.block_size << 4);

    sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size << 4) - 8;
    cr_assert(freelist_foot->alloc == 0);
    cr_assert(freelist_foot->block_size << 4 == 96, "Expected 96, was given: %d.", freelist_head->header.block_size << 4);

    cr_assert(freelist_head->next != NULL);
    cr_assert(freelist_head->prev == NULL);

    //check to see if the explicit list is intact
    sf_free_header* next = freelist_head->next;
    cr_assert(next->header.alloc == 0);
    cr_assert(next->header.block_size << 4 == 3936, "Expected 3936, was given: %d.", next->header.block_size << 4);

    cr_assert(next->next == NULL);
    cr_assert(next->prev == freelist_head);

    sf_footer* next_foot = (void*)next + (next->header.block_size << 4) - 8;
    cr_assert(next_foot->alloc == 0);
    cr_assert(next_foot->block_size << 4 == 3936, "Expected 3936, was given: %d.", next_foot->block_size << 4);
}

//2
Test(sf_memsuite, free_in_reverse, .init = sf_mem_init, .fini = sf_mem_fini) {
    //allocating some things and then freeing in reverse
    //make sure coalesce works
    sf_malloc(4);
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    sf_malloc(4);

    sf_free(c);
    sf_free(b);
    sf_free(a);

    cr_assert(freelist_head == a - 8);
    cr_assert(freelist_head->header.alloc == 0);
    cr_assert(freelist_head->header.block_size << 4 == 96, "Expected 96, was given: %d.", freelist_head->header.block_size << 4);

    sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size << 4) - 8;
    cr_assert(freelist_foot->alloc == 0);
    cr_assert(freelist_foot->block_size << 4 == 96, "Expected 96, was given: %d.", freelist_head->header.block_size << 4);

    cr_assert(freelist_head->next != NULL);
    cr_assert(freelist_head->prev == NULL);

    //check to see if the explicit list is intact
    sf_free_header* next = freelist_head->next;
    cr_assert(next->header.alloc == 0);
    cr_assert(next->header.block_size << 4 == 3936, "Expected 3936, was given: %d.", next->header.block_size << 4);

    cr_assert(next->next == NULL);
    cr_assert(next->prev == freelist_head);

    sf_footer* next_foot = (void*)next + (next->header.block_size << 4) - 8;
    cr_assert(next_foot->alloc == 0);
    cr_assert(next_foot->block_size << 4 == 3936, "Expected 3936, was given: %d.", next_foot->block_size << 4);
}

//3
Test(sf_memsuite, free_middle_last, .init = sf_mem_init, .fini = sf_mem_fini) {
    //freeing in an odd order
    sf_malloc(4);
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    sf_malloc(4);

    sf_free(a);
    sf_free(c);
    sf_free(b);

    cr_assert(freelist_head == a - 8);
    cr_assert(freelist_head->header.alloc == 0);
    cr_assert(freelist_head->header.block_size << 4 == 96, "Expected 96, was given: %d.", freelist_head->header.block_size << 4);

    sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size << 4) - 8;
    cr_assert(freelist_foot->alloc == 0);
    cr_assert(freelist_foot->block_size << 4 == 96, "Expected 96, was given: %d.", freelist_foot->block_size << 4);

    cr_assert(freelist_head->next != NULL);
    cr_assert(freelist_head->prev == NULL);

    //check to see if the explicit list is intact
    sf_free_header* next = freelist_head->next;
    cr_assert(next->header.alloc == 0);
    cr_assert(next->header.block_size << 4 == 3936, "Expected 3936, was given: %d.", next->header.block_size << 4);

    cr_assert(next->next == NULL);
    cr_assert(next->prev == freelist_head);

    sf_footer* next_foot = (void*)next + (next->header.block_size << 4) - 8;
    cr_assert(next_foot->alloc == 0);
    cr_assert(next_foot->block_size << 4 == 3936, "Expected 3936, was given: %d.", next_foot->block_size << 4);
}


//3.5
Test(sf_memsuite, free_middle_last_part_two, .init = sf_mem_init, .fini = sf_mem_fini) {
    //freeing in an odd order
    sf_malloc(4);
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    sf_malloc(4);

    sf_free(c);
    sf_free(a);
    sf_free(b);

    cr_assert(freelist_head == a - 8);
    cr_assert(freelist_head->header.alloc == 0);
    cr_assert(freelist_head->header.block_size << 4 == 96, "Expected 96, was given: %d.", freelist_head->header.block_size << 4);

    sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size << 4) - 8;
    cr_assert(freelist_foot->alloc == 0);
    cr_assert(freelist_foot->block_size << 4 == 96, "Expected 96, was given: %d.", freelist_foot->block_size << 4);

    cr_assert(freelist_head->next != NULL);
    cr_assert(freelist_head->prev == NULL);

    //check to see if the explicit list is intact
    sf_free_header* next = freelist_head->next;
    cr_assert(next->header.alloc == 0);
    cr_assert(next->header.block_size << 4 == 3936, "Expected 3936, was given: %d.", next->header.block_size << 4);

    cr_assert(next->next == NULL);
    cr_assert(next->prev == freelist_head);

    sf_footer* next_foot = (void*)next + (next->header.block_size << 4) - 8;
    cr_assert(next_foot->alloc == 0);
    cr_assert(next_foot->block_size << 4 == 3936, "Expected 3936, was given: %d.", next_foot->block_size << 4);
}

//4
Test(sf_memsuite, free_whole_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *a = sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(5000);

    sf_free(a);
    sf_free(c);
    sf_free(b);

    cr_assert(freelist_head == a - 8);
    cr_assert(freelist_head->header.alloc == 0);
    cr_assert(freelist_head->header.block_size << 4 == 8192, "Expected 8192, was given: %d.", freelist_head->header.block_size << 4);

    sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size << 4) - 8;
    cr_assert(freelist_foot->alloc == 0);
    cr_assert(freelist_foot->block_size << 4 == 8192, "Expected 8192, was given: %d.", freelist_foot->block_size << 4);

    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}



//5
Test(sf_memsuite, malloc_then_free_all_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
    //seeing if this breaks malloc correctly
    void* a = sf_malloc(16337);

    void* nullpls = sf_malloc(32);
    cr_assert(nullpls == NULL);
    cr_assert(errno == ENOMEM);
    errno = 0;

    sf_free_header *ahead = a - 8;
    cr_assert(ahead->header.alloc == 1);
    cr_assert(ahead->header.block_size << 4 == 16384, "Expected 16384, was given: %d.", ahead->header.block_size << 4);

    sf_free(a);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);

    sf_malloc(4);
    void *b = sf_malloc(4);
    void *c = sf_malloc(4);
    sf_free(b);

    cr_assert((sf_free_header*)freelist_head->next == c - 8 + 32);
}



//6
Test(sf_memsuite, please_refrain_from_adding_more_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
    //seeing if it doesnt add a oage
    cr_assert(errno == 0);
    void* a = sf_malloc(4080);

    cr_assert(freelist_head == NULL);
    sf_free(a);
    void* b = sf_malloc(2048 - 16);
    void* c = sf_malloc(1024 - 16);
    void* d = sf_malloc(1024 - 16);
    cr_assert(freelist_head == NULL);
    sf_free(b);
    sf_free(d);
    sf_free(c);

    cr_assert(freelist_head->header.block_size << 4 == 4096);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}

//7
Test(sf_memsuite, test_taking_out_of_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
    //just some more freeing, this time with a full page (NULL freelist head)
    sf_malloc(32);
    int* a = sf_malloc(32);
    int* b = sf_malloc(32);
    int* c = sf_malloc(32);
    sf_malloc(32);
    sf_malloc(3840);
    cr_assert(freelist_head == NULL);

    sf_free(a);
    sf_free(c);
    sf_free(b);
    cr_assert(freelist_head == ((void*)a) - 8);
    cr_assert(freelist_head->next == NULL);

}


//8
Test(sf_memsuite, test_some_small_reallocs, .init = sf_mem_init, .fini = sf_mem_fini) {
    //testing reallocs when the size is smaller
    void *b = sf_malloc(128);
    sf_realloc(b, 10);
    cr_assert(((sf_header*)(b - 8))->block_size << 4 == 32);
    cr_assert(((sf_header*)(b - 8))->padding_size == 6);
    cr_assert(freelist_head->header.block_size << 4 == 4064);

    sf_realloc(b, 1);
    cr_assert(((sf_header*)(b - 8))->block_size << 4 == 32);
    cr_assert(((sf_header*)(b - 8))->padding_size == 15);
    cr_assert(freelist_head->header.block_size << 4 == 4064);

    sf_free(b);
    cr_assert(b - 8 == freelist_head);
    cr_assert(freelist_head->header.block_size << 4 == 4096);

}

//9
Test(sf_memsuite, test_some_big_reallocs, .init = sf_mem_init, .fini = sf_mem_fini) {
    //testing reallocs when it's bigger
    sf_malloc(32);
    int *a = sf_malloc(32);
    int* b = sf_malloc(32);
    sf_malloc(32);
    int *c =
        sf_malloc(32);
    int* d =
        sf_malloc(32);
    sf_malloc(32);
    sf_malloc(3744); //4096

    //splinter
    cr_assert(freelist_head == NULL);
    sf_free(b);
    sf_realloc(a, 64); //80 + 16 for splinter
    cr_assert(freelist_head == NULL);


    //gotta malloc
    sf_free(c);
    sf_realloc(d, 128);
    cr_assert(freelist_head == (void*)c - 8);

    sf_malloc(80);
    sf_malloc(64);
    void* e = sf_malloc(64);
    void *f = sf_malloc(64);
    sf_malloc(64);
    sf_malloc(64);

    //extend the header
    sf_free(f);
    e = sf_realloc(e, 90);

    cr_assert(freelist_head == (e - 8) + 112);
    cr_assert(freelist_head->header.block_size << 4 == 48);
    cr_assert(((sf_free_header *)(e - 8))->header.block_size << 4 == 112);
}


//10
Test(sf_memsuite, more_realloc_tests, .init = sf_mem_init, .fini = sf_mem_fini) {
    //seeing if realloc can set everything to null right
    sf_malloc(32);
    void *a = sf_malloc(32);
    void* b = sf_malloc(32);
    sf_malloc(32);
    sf_malloc(32);


    sf_realloc(a, 256);
    cr_assert(freelist_head == a - 8);

    sf_free(b);
    cr_assert(freelist_head == a - 8);
    cr_assert(freelist_head->next->next == NULL);

}


//11
Test(sf_memsuite, realloc_stress_test, .init = sf_mem_init, .fini = sf_mem_fini) {
    //testing to see if the memory all lines up
    void *a = sf_malloc(32);    //48
    void* b = sf_malloc(32);    //48
    void* c = sf_malloc(32);    //48
    void *d = sf_malloc(4);         //32
    //176


    a = sf_realloc(a, 48);      //wont fit, get freed, allocate 64 after the 32
    //64
    b = sf_realloc(b, 48);      //wont fit, get freed, allocate 64 after the 32
    //64

    c = sf_realloc(c, 4);       //will fit in a
    //32 bytes

    size_t total = (((sf_free_header*)(a - 8))->header.block_size << 4);
    total += (((sf_free_header*)(b - 8))->header.block_size << 4);
    total += (((sf_free_header*)(c - 8))->header.block_size << 4);
    total += (((sf_free_header*)(d - 8))->header.block_size << 4);
    total += (freelist_head->header.block_size << 4);
    total += (freelist_head->next->header.block_size << 4);

    cr_assert(total == 4096);

}


//12
Test(sf_memsuite, crashing_this_malloc, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* a = sf_malloc(20000);
    cr_assert(a == NULL);
    cr_assert(errno != 0);
    errno = 0;
}

//13
Test(sf_memsuite, realloc_value_test, .init = sf_mem_init, .fini = sf_mem_fini) {
    //see if i can actually use realloc to copy memory
    sf_malloc(32);
    long double *a = sf_malloc(32);
    *a = 100;
    long double* b = sf_malloc(32);
    *b = 31;
    sf_malloc(32);
    cr_assert(*a == 100);
    cr_assert(*b == 31);

    b = realloc(b, 128);
    cr_assert(*b == 31);


    *b += *a;
    sf_free(a);

    b = realloc(b, 16);
    cr_assert(*b == 31 + 100);
}



//14
Test(sf_memsuite, test_decouple, .init = sf_mem_init, .fini = sf_mem_fini) {
    //SEE IF MY LIST STAYS INTACT
    sf_malloc(4);
    sf_malloc(4);
    void* c = sf_malloc(16);
    sf_malloc(64);
    void* d = sf_malloc(128);
    sf_malloc(256);
    void* e = sf_malloc(4);
    sf_malloc(4);

    sf_free(c);
    sf_free(d);
    sf_free(e);

    sf_malloc(32);

//  for(sf_free_header *freeb = freelist_head; freeb!=NULL; freeb=freeb->next)
//  { sf_blockprint(freeb);}

}


//15
Test(sf_memsuite, test_sf_info, .init = sf_mem_init, .fini = sf_mem_fini) {
    //testing sf_info returns
    printf("START\n");
    sf_malloc(4);
    void* a = sf_malloc(16);
    a = sf_realloc(a, 32);

    info k;
    sf_info(&k);
    printf("internal:%li\n", k.internal);
    printf("external:%li\n", k.external);
    printf("allocations:%li\n", k.allocations);
    printf("frees:%li\n", k.frees);
    printf("coalesce:%li\n", k.coalesce);
    printf("END\n");
}


//16
Test(sf_memsuite, test_realloc_splinter_when_next_block_is_free, .init = sf_mem_init, .fini = sf_mem_fini) {
    //testing if the 16 bytes are merged correctly
    sf_malloc(4);
    void* a = sf_malloc(32);
    a = sf_realloc(a, 16);
    cr_assert(freelist_head->header.block_size << 4 == 4032);


    //now see if it just splinters
    sf_free(a);//32
    a = sf_malloc(32);//32 + _48
    sf_malloc(4);//80 + _32
    a = sf_realloc(a, 16); //112 still _48 because splinter
    cr_assert(((sf_free_header*)(a - 8))->header.block_size << 4 == 48);
    cr_assert(freelist_head->header.block_size << 4 == 4016 - 32);
}

//17
Test(sf_memsuite, test_freeing_outside_our_heap, .init = sf_mem_init, .fini = sf_mem_fini) {
    //testing to see if freeing outside of the heap fails (set errno ya dingus)
    void* a = sf_malloc(32);
    sf_free(a - 8);
    cr_assert(errno != 0);
    errno = 0;
}

//y_sfunit.c

Test(sf_memsuite, Alignment_and_Pages_Check, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(32); // Block will be 32 + 16
    void* y = sf_malloc(4016); // Free block is 4048 before malloc
    void* z = sf_malloc(32);
    memset(x, 1, 32);
    memset(y, 2, 4000);
    memset(z, 3, 32);
    cr_assert(freelist_head == (z + 40));
    sf_free(x);
    cr_assert(((sf_header*)(y - 8))->block_size << 4 == 4048);
    cr_assert((size_t)z % 16 == 0, "Payload Not Aligned");
}

Test(sf_memsuite, Two_Page_Allocation, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(4097);
    memset(x, 1, 4097);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == (4096 * 2));
}

Test(sf_memsuite, Three_Page_Allocation, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(8193);
    memset(x, 1, 8193);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == (4096 * 3));
}

Test(sf_memsuite, Four_Page_Allocation, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(4096 * 3 + 1);
    memset(x, 1, 4096 * 3 + 1);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == (4096 * 4));
}

Test(sf_memsuite, Five_Page_Allocation, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* x = sf_malloc(4096 * 4);
    sf_free(x);
    cr_assert(x == NULL);
}
Test(sf_memsuite, Full_Page_Allocation_Splinter_and_free, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4064);
    memset(x, 2, 4000);
    cr_assert(freelist_head == NULL);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == 4096);
}

Test(sf_memsuite, Full_Page_Allocation_and_free, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(4080);
    memset(x, 2, 4000);
    cr_assert(freelist_head == NULL);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == 4096);
}

Test(sf_memsuite, Splinter_Allocation_and_free, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *x = sf_malloc(32); // Block will be 32 + 16
    void* y = sf_malloc(4016); // Free block is 4048 before malloc
    memset(x, 1, 32);
    memset(y, 2, 4000);
    cr_assert(((sf_header*)(y - 8))->block_size << 4 == 4048);
}

Test(sf_memsuite, Coalesce_prev_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    cr_assert(freelist_head == (z - 8 + 144));
    sf_free(x);
    sf_free(y);
    cr_assert(freelist_head == (x - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    cr_assert(freelist_head->header.block_size << 4 == 192);
    cr_assert(freelist_head->next == (z - 8 + 144));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_next_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    cr_assert(freelist_head == (z - 8 + 144));
    sf_free(y);
    sf_free(x);
    cr_assert(freelist_head == (x - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    cr_assert(freelist_head->header.block_size << 4 == 192);
    cr_assert(freelist_head->next == (z - 8 + 144));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_both_block_1, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    cr_assert(freelist_head == (z - 8 + 144));
    sf_free(w); //48
    sf_free(y); //112
    sf_free(x); //48+112+80 = 240
    cr_assert(freelist_head == (w - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    cr_assert(freelist_head->header.block_size << 4 == 240);
    cr_assert(freelist_head->next == (z - 8 + 144));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_both_block_2, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    cr_assert(freelist_head == NULL);
    sf_free(s); //112
    sf_free(hi); //112+80 = 192
    sf_free(bye);
    cr_assert(freelist_head == (hi - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_both_block_3, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    cr_assert(freelist_head == NULL);
    sf_free(bye); //112
    sf_free(hi); //112+80 = 192
    sf_free(s);
    cr_assert(freelist_head == (hi - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_both_block_4_Basic, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    cr_assert(freelist_head == NULL);
    sf_free(bye); //112
    sf_free(z); //112+80 = 192
    sf_free(hi);
    cr_assert(freelist_head == (z - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Realloc_Actual_Splinter_Case, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    void* w_ftr = w - 16 + (((sf_header*)(w - 8))->block_size << 4);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    cr_assert(freelist_head == NULL);
    void* w2 = sf_realloc(w, 12);
    void* n_ftr = w - 16 + (((sf_header*)(w - 8))->block_size << 4);
    cr_assert(w == w2, "Realloc Splinter Header Issues");
    cr_assert(w_ftr == n_ftr, "Realloc Splinter Footer Issues");
    cr_assert((((sf_header*)(w2 - 8))->block_size << 4) == 48);
    sf_free(bye); //112
    sf_free(z); //112+80 = 192
    sf_free(hi);
    cr_assert(freelist_head == (z - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Realloc_Perfect_Fit, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    void* w_ftr = w - 16 + (((sf_header*)(w - 8))->block_size << 4);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    cr_assert(freelist_head == NULL);
    void* w2 = sf_realloc(w, 32);
    void* n_ftr = w - 16 + (((sf_header*)(w - 8))->block_size << 4);
    cr_assert(w == w2, "Realloc Splinter Header Issues");
    cr_assert(w_ftr == n_ftr, "Realloc Splinter Footer Issues");
    cr_assert((((sf_header*)(w2 - 8))->block_size << 4) == 48);
    sf_free(bye); //112
    sf_free(z); //112+80 = 192
    sf_free(hi);
    cr_assert(freelist_head == (z - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Realloc_Search_Case_Not_Next_to_Free, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    cr_assert(freelist_head == NULL);
    void* new_x = sf_realloc(x, 128);
    void* flhNext = new_x - 8 + (((sf_header*)(new_x - 8))->block_size << 4);
    memset(new_x, 8, 128);
    cr_assert(freelist_head == x - 8, "Incorrect Freelist Head"); //Correctly updated freelist head
    cr_assert(freelist_head->next == flhNext);
    sf_free(bye); //112
    sf_free(z); //112+80 = 192
    sf_free(hi);
    sf_snapshot(true);
    cr_assert(freelist_head == (z - 8));
    cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Realloc_Fake_Splinter_Case_Coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    //void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    //memset(s, 7, 2896);
    void* new_bye = sf_realloc(bye, 128);
    memset(new_bye, 8, 128); //528 - 144
    cr_assert((freelist_head->header.block_size << 4) == (2912 + 528 - 144));
    //sf_free(bye); //112
    //sf_free(z); //112+80 = 192
    //sf_free(hi);
    sf_snapshot(true);
    // cr_assert(freelist_head==(z-8));
    // cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Realloc_Fake_Splinter_Case, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    //void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    //memset(s, 7, 2896);
    void* new_bye = sf_realloc(bye, 128);
    memset(new_bye, 8, 128); //528 - 144
    cr_assert((freelist_head->header.block_size << 4) == (2912 + 528 - 144));
    //sf_free(bye); //112
    //sf_free(z); //112+80 = 192
    //sf_free(hi);
    sf_snapshot(true);
    // cr_assert(freelist_head==(z-8));
    // cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Realloc_No_Splinter_No_Coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *w = sf_malloc(32);
    void *x = sf_malloc(64);
    void *y = sf_malloc(96);
    void *z = sf_malloc(128);
    void *hi = sf_malloc(256);
    void *bye = sf_malloc(512); //2912 bytes left
    void* s = sf_malloc(2896);
    memset(w, 1, 32);
    memset(x, 2, 64);
    memset(y, 3, 96);
    memset(z, 4, 128);
    memset(hi, 5, 256);
    memset(bye, 6, 512);
    memset(s, 7, 2896);
    sf_free(hi);
    cr_assert(freelist_head == hi - 8, "Incorrect Header after free");
    void* new_bye = sf_realloc(bye, 128);
    memset(new_bye, 8, 128); //528 - 144
    cr_assert((freelist_head->header.block_size << 4) == (528 - 144));
    cr_assert(freelist_head->next == hi - 8);
    //sf_free(bye); //112
    //sf_free(z); //112+80 = 192
    //sf_free(hi);
    // sf_snapshot(true);
    // cr_assert(freelist_head==(z-8));
    // cr_assert(freelist_head->header.padding_size == 0);
    // cr_assert(freelist_head->header.block_size << 4 == 192);
    // cr_assert(freelist_head->next == (bye-8+528));
    cr_assert(freelist_head->prev == NULL);
}

//s_sfunit.c

Test(sf_memsuite, Coalesce_next_and_prev, .init = sf_mem_init, .fini = sf_mem_fini) {

    /* This unit test will free the prev and next and keep an allocated*/
    /* In the middle. Then we will free that middle one and coalesce the whole block*/
    int *x = sf_malloc(sizeof(int));
    char *y = sf_malloc(sizeof(char));
    long *z = sf_malloc(sizeof(long));
    short *v = sf_malloc(sizeof(short));

    *v = 2;

    sf_free(x);
    sf_free(z);
    sf_free(y); // COALESCING SHOULD ONLY HAPPEN HERE-> NEXT AND PREVIOUS


    sf_free_header* free_ptr = (sf_free_header*)((char*)x - 8);
    cr_assert(free_ptr->header.block_size << 4 == 96);
    cr_assert(freelist_head == (void*)free_ptr);
    cr_assert((freelist_head->next)->header.block_size << 4 == 3968);

    info memo = {0, 0, 0, 0, 0};

    info* meminfo = &memo;
    sf_info(meminfo);
    cr_assert(meminfo->coalesce == 1); // checks if we only counted this instance as ONE COALESCE
    cr_assert(meminfo->frees == 3);
    cr_assert(meminfo->allocations == 4);
    cr_assert(meminfo->external == 4064);
    cr_assert(meminfo->internal == 30);
    // This test checks coalescing if the next and previous blocks are free.
}

Test(sf_memsuite, Avoiding_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {

    // X and Y will be both 32 byte blocks. Freeing both of them
    // and then coalescing will give on 64 byte block.
    // sf_malloc(20) will create a 48 byte bloc, which will cause a splinter
    // hence, we will absorb the splinter and create on big 64 byte block.
    void *x = sf_malloc(10);
    void *y = sf_malloc(10);
    char *z = sf_malloc(60);

    *z = 10;

    sf_free(x);
    sf_free(y);
    void* check = sf_malloc(20);

    sf_header* header = check - 8;

    cr_assert(header->block_size << 4 == 64);
    cr_assert(header->padding_size == 12); // shows padding does not take into account the extra space to avoid the splinter
    cr_assert(freelist_head->header.block_size << 4 == 3952);

    info memo = {0, 0, 0, 0, 0};

    info* meminfo = &memo;
    sf_info(meminfo);

    cr_assert(meminfo->allocations == 4);
    cr_assert(meminfo->frees == 2);
    cr_assert(meminfo->internal == 48);
    cr_assert(meminfo->external == 3952);

}
Test(sf_memsuite, Realloc_shrink, .init = sf_mem_init, .fini = sf_mem_fini) {

// Realloc will shrink the current allocated block, and then split it and create a new free block.
    int *x = sf_malloc(4);
    int *y = sf_malloc(42); // this block should be 64 bytes
    int *z = sf_malloc(4);

    *x = 3;
    *y = 17;
    *z = 4;

    y = sf_realloc(y, 4); // the new block will be 32 bytes

    cr_assert(*y == 17); // check if value is retained.
    sf_header* y_header = (sf_header*)((char*)y - 8); // get the header of y

    size_t blocksize = y_header->block_size << 4;

    sf_free_header* free_ptr = (sf_free_header*)((char*)y_header + blocksize); // get the header of the newly created free_ptr

    cr_assert(y_header->block_size << 4 == 32); //check if the allocated block is 32 bytes
    cr_assert(freelist_head == free_ptr);
    cr_assert(free_ptr->header.alloc == 0);
    cr_assert(free_ptr->header.block_size << 4 == 32);
}
Test(sf_memsuite, Realloc_grow_find_new_block, .init = sf_mem_init, .fini = sf_mem_fini) {

    int *x = sf_malloc(4);
    int *y = sf_malloc(4);
    int *z = sf_malloc(4);


    *x = 10;
    *y = 20;
    *z = 30;
    sf_header* y_header = (sf_header*)((char*)y - 8); // this is the original header of y

    sf_free_header* address_of_freelist_head = freelist_head;

    y = sf_realloc(y, 20); // the new block size should be 48, WONT fit in the current


    sf_header* new_y_header = (sf_header*)((char*)y - 8);

    cr_assert(new_y_header->block_size << 4 == 48);
    cr_assert(y_header->alloc == 0); // check if the header that y used to be in is free
    cr_assert((void*)new_y_header == (void*)address_of_freelist_head); // check if the address of the new y is the address of the old freelist head
    cr_assert(*y == 20); /// check if the value was retained

}

Test(sf_memsuite, Realloc_find_new_not_enough_space_next, .init = sf_mem_init, .fini = sf_mem_fini) {

    char *x = sf_malloc(4);
    int *y = sf_malloc(4);
    int *z = sf_malloc(4);

    *z = 10;
    *x = 'd';

    sf_header* x_header = (sf_header*)((char*)x - 8); // this is the original header of y
    sf_free_header* original_freelist_head = freelist_head;

//sf_header* y_header= (sf_header*)((char*)y-8);
    sf_free(y); // x is allocated, y is not. x+y= 64 bytes

//cr_assert(freelist_head==(void*)y_header);
    x = sf_realloc(x, 70); // block should now be 80 bytes, which would not fit in the current x even though there is space next

    sf_header* new_x_header = (sf_header*)((char*)x - 8);

    cr_assert(x_header->alloc == 0);
    cr_assert(x_header->block_size << 4 == 64); // check if x and y coalesced.
    cr_assert(*x == 'd'); // check if the payload is still the same
    cr_assert(new_x_header == (void*)original_freelist_head);

}

Test(sf_memsuite, Realloc_coalesce_avoid_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {

// In this case, there is a splinter. Instead of just padding the block size, we coalesce the remaining splinter with the adjacent free block
    int *y = sf_malloc(20);
    *y = 300;
    sf_realloc(y, 4);

    cr_assert(*y == 300); // check if the value is retained.

    sf_header* y_header = (sf_header*)((char*)y - 8);

    cr_assert(y_header->block_size << 4 == 32);
    cr_assert(freelist_head->header.block_size << 4 == 4064);

}

Test(sf_memsuite, Free_out_of_bounds, .init = sf_mem_init, .fini = sf_mem_fini) {

    int *x = sf_malloc(4);
    sf_free((void*)x - 16); // THIS GOES OUT OF BOUNDS

    sf_header* x_header = (sf_header*)((void*)x - 8);
    cr_assert(x_header->alloc == 1);
    cr_assert(x_header->block_size << 4 == 32);
    cr_assert(freelist_head->header.block_size << 4 == 4064); // SHOWS THAT EVERYTHING IS STILL THE SAME


    info memo = {0, 0, 0, 0, 0};

    info* meminfo = &memo;
    sf_info(meminfo);

    cr_assert(meminfo->frees == 0); // check if free is still good
}

Test(sf_memsuite, Not_enough_heap_space, .init = sf_mem_init, .fini = sf_mem_fini) {

    int *x = sf_malloc(16352);
    sf_varprint(x);
    int *y = sf_malloc(32);
    cr_assert(y == NULL);
    sf_header* x_header = (sf_header*)((void*)x - 8);
    cr_assert(x_header->block_size << 4 == 16384);
    cr_assert(freelist_head == NULL);
    cr_assert(errno != 0);

    info memo = {0, 0, 0, 0, 0};

    info* meminfo = &memo;
    sf_info(meminfo);

    cr_assert(meminfo->external == 0);

    printf("ALLOCATIONS %d\n", (int)meminfo->internal);
    // cr_assert(meminfo->internal==32);
    cr_assert(meminfo->allocations == 1);
}

//j_sfunit.c

Test(sf_memsuite, Malloc_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 1, "Alloc bit in header is not 1!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 1, "Alloc bit in the footer is not 1!\n");
}

Test(sf_memsuite, Realloc_block_check_header_footer_size_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    void* realloc_pointer = sf_realloc(pointer, 48);
    realloc_pointer = realloc_pointer - 8;
    sf_header *sfHeader = (sf_header *) realloc_pointer;
    cr_assert(sfHeader->alloc == 1, "Alloc bit in header is not 1!\n");
    sf_footer *sfFooter = (sf_footer *) (realloc_pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 1, "Alloc bit in the footer is not 1!\n");
    cr_assert((sfHeader->block_size << 4) == 64, "ReAlloced block_size not 64!");
}

Test(sf_memsuite, Alloc_large_block_check_header_footer_size_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(12000);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 1, "Alloc bit in header is not 1!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 1, "Alloc bit in the footer is not 1!\n");
    cr_assert((sfHeader->block_size << 4) == 12016, "Block_size not 12,016!");
}

Test(sf_memsuite, Alloc_block_then_alloc_too_large_block_check_header_footer_too_large_pointer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(12000);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 1, "Alloc bit in header is not 1!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 1, "Alloc bit in the footer is not 1!\n");
    cr_assert((sfHeader->block_size << 4) == 12016, "Block_size not 12,016!");
    void *tooMuchPointer = sf_malloc(4500);
    cr_assert(tooMuchPointer == NULL, "Pointer is not NULL!\n");
}

Test(sf_memsuite, Realloc_too_large_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    void* realloc_pointer = sf_realloc(pointer - 8, 16384);
    pointer = pointer - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;
    cr_assert(sfHeader1->alloc == 1, "Alloc bit in header is not 1!\n");
    sf_footer *sfFooter1 = (sf_footer *) (pointer - 8 + (sfHeader1->block_size << 4));
    cr_assert(sfFooter1->alloc == 1, "Alloc bit in the footer is not 1!\n");
    cr_assert((sfHeader1->block_size << 4) == 32, "Block_size not 32!");
    cr_assert(realloc_pointer == NULL, "Pointer is not NULL!\n");
}

Test(sf_memsuite, Alloc_two_blocks_then_realloc_first_one_too_large_check_header_footer_values_of_each, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(sizeof(short));
    void* pointer2 = sf_malloc(64);
    void* realloc_pointer = sf_realloc(pointer, 16358);
    pointer = pointer - 8;
    pointer2 = pointer2 - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;
    sf_header* sfHeader2 = (sf_header *) pointer2;
    cr_assert(sfHeader1->alloc == 1, "Alloc bit in header is not 1!\n");
    cr_assert((sfHeader1->block_size << 4) == 32, "Block_size not 32!");
    pointer = pointer - 8 + (sfHeader1->block_size << 4);
    sf_footer *sfFooter1 = (sf_footer *) pointer;
    cr_assert(sfFooter1->alloc == 1, "Alloc bit in the footer is not 1!\n");

    cr_assert(sfHeader2->alloc == 1, "Alloc bit in header is not 1!\n");
    cr_assert((sfHeader2->block_size << 4) == 80, "Block_size not 80!");
    pointer2 = pointer2 - 8 + (sfHeader2->block_size << 4);
    sf_footer *sfFooter = (sf_footer *) pointer2;
    cr_assert(sfFooter->alloc == 1, "Alloc bit in the footer is not 1!\n");

    cr_assert(realloc_pointer == NULL, "Pointer is not NULL!\n");
}

Test(sf_memsuite, Free_block_from_wrong_ptr_does_not_break, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(1008);
    sf_free(pointer-5);
    pointer = pointer - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;
    cr_assert(sfHeader1->alloc == 1, "Alloc bit in header is not 1!\n");
    cr_assert((sfHeader1->block_size << 4) == 1024, "Block_size not 32!");
    pointer = pointer - 8 + (sfHeader1->block_size << 4);
    sf_footer *sfFooter1 = (sf_footer *) pointer;
    cr_assert(sfFooter1->alloc == 1, "Alloc bit in the footer is not 1!\n");
}

Test(sf_memsuite, Coalesce_next_and_prev_1, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(1008);
    void* pointer2 = sf_malloc(1008);

    pointer = pointer - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;

    sf_free(pointer+8);
    sf_free(pointer2);

    cr_assert(sfHeader1->alloc == 0, "Alloc bit in header is not 0!\n");
    cr_assert((sfHeader1->block_size << 4) == 4096, "Block_size not 4096!");
    pointer = pointer - 8 + (sfHeader1->block_size << 4);
    sf_footer* sfFooter1 = (sf_footer *) pointer;
    cr_assert(sfFooter1->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, Coalesce_next, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(150);

    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;
    cr_assert(sfHeader1->alloc == 0, "Alloc bit in header is not 0!\n");
    cr_assert((sfHeader1->block_size << 4) == 4096, "Block_size not 4096!");
    pointer = pointer - 8 + (sfHeader1->block_size << 4);
    sf_footer* sfFooter1 = (sf_footer *) pointer;
    cr_assert(sfFooter1->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, Coalesce_prev, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(150);
    void* pointer1 = sf_malloc(250);
    sf_malloc(69);

    sf_free(pointer);
    sf_free(pointer1);
    pointer = pointer - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;
    cr_assert(sfHeader1->alloc == 0, "Alloc bit in header is not 0!\n");
    cr_assert((sfHeader1->block_size << 4) == 448, "Block_size not 448!");
    pointer = pointer - 8 + (sfHeader1->block_size << 4);
    sf_footer* sfFooter1 = (sf_footer *) pointer;
    cr_assert(sfFooter1->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, Test_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(150);
    void* tempFreePointer = pointer-8 + 176;
    sf_free_header* freeList_head = tempFreePointer;

    cr_assert(freelist_head==freeList_head, "The freelist_head is incorrect!!");

    sf_free(pointer);
    cr_assert(freelist_head==pointer-8, "The freelist_head is incorrect!!");
}

Test(sf_memsuite, realloc_spliter_test_and_check_freelist_head_updated, .init = sf_mem_init, .fini = sf_mem_fini) {
    void* pointer = sf_malloc(150);
    void* pointer1 = sf_malloc(250);
    sf_malloc(69);

    sf_free(pointer1);
    sf_realloc(pointer, 420);
    pointer = pointer - 8;
    sf_header *sfHeader1 = (sf_header *) pointer;
    cr_assert(sfHeader1->alloc == 1, "Alloc bit in header is not 0!\n");
    cr_assert((sfHeader1->block_size << 4) == 448, "Block_size not 448!");
    pointer = pointer - 8 + (sfHeader1->block_size << 4);
    sf_footer* sfFooter1 = (sf_footer *) pointer;
    cr_assert(sfFooter1->alloc == 1, "Alloc bit in the footer is not 0!\n");

    sf_free_header* testFreeList = pointer + 8 + 96;
    cr_assert(testFreeList == freelist_head, "The freelist_head is incorrect");
}


Test(sf_memsuite, realloc_moved_to_new_block_check_value, .init = sf_mem_init, .fini = sf_mem_fini) {
    int* pointer = sf_malloc(sizeof(int));
    void* tempPointer = pointer;
    sf_malloc(4);

    *pointer = 52;
    int pointerValue = *pointer;
    void* newLoc = sf_realloc(pointer, 32);
    int* newintLoc = newLoc;
    cr_assert(newLoc == 64+tempPointer, "newLoc is not where it should be!!: Pointer location: %p newIntLoc: %p != %p",pointer, newLoc, tempPointer +64);
    cr_assert(*newintLoc==52, "The value at the new location is not correct!! newIntValue: %d pointer value: %d", *newintLoc, pointerValue);
}


Test(sf_memsuite, Alloc_too_large_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(16384);
    cr_assert(pointer == NULL, "Pointer is not NULL!\n");
}

//e_sfunit.c

 #define SHIFT(p)((p) >> 4)
 #define SHIFT_BACK(p)((p) << 4)
 #define GET_PAYLOAD(block_size)((block_size) - 16)
 #define GO_TO_FOOTER(adress, block_size)((adress) + GET_PAYLOAD(block_size) + 8)
 #define BACK_TO_HEADER(adress, block_size)((adress) - GET_PAYLOAD(block_size) - 8)
 #define GO_UP(address)((void*)(address) + 8)
 #define GO_DOWN(address)((void*)(address) - 8)
 #define PAGE_SIZE 4096
 #define SMALLEST_BLOCK_SIZE 32

Test(sf_memsuite, Realloc_from_long_to_int, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr = sf_malloc(sizeof(long));
    sf_header* current_header = GO_DOWN(ptr);
    sf_blockprint((void*) current_header);
    int old_block_size = current_header->block_size;

    void* new_ptr = sf_realloc(ptr, sizeof(int));
    cr_assert(ptr == new_ptr, "FAILED: Pointers are not equal after calling realloc.");
    current_header = GO_DOWN(new_ptr);
    sf_blockprint((void*) current_header);

    cr_assert(current_header->padding_size == 12, "Padding size didn't change to 12.");
    cr_assert(current_header->block_size == old_block_size,
        "Block sizes are not the same after only changing the padding");
}

Test(sf_memsuite, Realloc_into_bigger_block_merging, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr1 = sf_malloc(100);
    void* ptr2 = sf_malloc(100);
    void* ptr3 = sf_malloc(100);
    void* ptr4 = sf_malloc(100);

    sf_header* current_header = GO_DOWN(ptr1);
    sf_blockprint((void*)current_header);
    cr_assert(SHIFT_BACK(current_header->block_size) == 128, 
        "Size is not 128 for allocated blocks");
    cr_assert(current_header->padding_size == 12, "Incorrect padding.");
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 3584
        , "Free block is not the correct size after malloc.");

    sf_free(ptr1);
    sf_free(ptr3);
    sf_snapshot(true);

    void* old_ptr = ptr2;
    sf_realloc(ptr2, 150);
    cr_assert(ptr2 == old_ptr, "Pointers not equal after assert");
    current_header = GO_DOWN(ptr2);
    cr_assert(SHIFT_BACK(current_header->block_size) == 176,
        "Realloc did not return a block size of 176");
    cr_assert(current_header->padding_size == 10, "Padding is not correct");

    sf_snapshot(true);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 80,
        "Size of head of list not 80.");
    cr_assert(freelist_head->header.padding_size == 0,
        "Padding size of free block is not 0.");

    sf_blockprint((void*)freelist_head);
    sf_free(ptr4);
    sf_snapshot(true);
    sf_blockprint((void*)freelist_head);
}

Test(sf_memsuite, Coalesce_last_remaining_cases, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr1 = sf_malloc(17);
    void* ptr2 = sf_malloc(17);
    void* ptr3 = sf_malloc(17);
    void* ptr4 = sf_malloc(17);
    void* ptr5 = sf_malloc(17);
    void* ptr6 = sf_malloc(17);

    sf_free(ptr2);
    sf_free(ptr4);
    cr_assert(SHIFT_BACK(freelist_head->next->next->header.block_size) == 3808, 
        "Tail of the free list got the wrong size.");
    sf_varprint(ptr2);
    sf_snapshot(true);

    sf_free(ptr5);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 96, 
        "Head of the free list got the wrong size.");
    cr_assert(freelist_head->header.padding_size == 0,
        "Padding was not set to 0 when freeing.");

    sf_free(ptr1);
    sf_snapshot(true);

    sf_free(ptr3);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 240, 
        "head of the free list got the wrong size.");

    sf_header* header = ptr6 - 8;
    cr_assert(header->padding_size = 15);
    sf_free(ptr6);

    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 4096, 
        "head of the free list got the wrong size after freeing everything.");
}

Test(sf_memsuite, Realloc_16_bytes_edgecase, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr = sf_malloc(17);
    sf_header* header = ptr -8; 
    cr_assert(header->padding_size == 15,"Padding wasn't corect");
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 4048,
        "Block is nt 4048.");
    ptr = sf_realloc(ptr,4);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 4064,
        "Block is nt 4064.");

    cr_assert(header->padding_size == 12,"Padding wasn't corect");
}

Test(sf_memsuite, Realloc_best_case, .init = sf_mem_init, .fini = sf_mem_fini){
    long* ptr1 = sf_malloc(73);
    void* ptr2 = sf_malloc(73);
    *ptr1 = 123456789;
    long value = 123456789;

    sf_header* header = (void*)ptr1 - 8;
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 3904);
    cr_assert(header->padding_size == 7, "Padding is wrong.");

    void* new_ptr = sf_realloc(ptr1, 158);
    cr_assert(*((long*)new_ptr) == value, 
        "Value was not copied correctly during realloc.");
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 96);
    cr_assert(SHIFT_BACK(freelist_head->next->header.block_size) == 3728);
    sf_blockprint(freelist_head);

    sf_free(ptr2);
    cr_assert(ptr1 == (void*) freelist_head + 8, "Freelist head in the wrong address.");
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 192);

}

Test(sf_memsuite, Realloc_size_zero, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr = sf_malloc(500);
    void* ptr2 = sf_realloc(ptr, 0);

    cr_assert(ptr2 == NULL, "Should've returned null");
    cr_assert(errno != 0, "errno not set to the right value");
}


Test(sf_memsuite, Malloc_madness, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr_ar[512];

    for (int i = 0; i < 512; ++i){
        ptr_ar[i] = sf_malloc(16);
    }

    cr_assert(freelist_head == NULL, "Everything should've been allocated");

    for (int i = 0; i < 512; ++i){
        sf_free(ptr_ar[i]);
    }

    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 16384, 
        "Not everything was freed.");
}


Test(sf_memsuite, Malloc_size_zero, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr = sf_malloc(0);
    errno = 0;
    cr_assert(ptr == NULL, "Should've returned null");
    cr_assert(errno == 0, "errno not set to the right value");

    errno = 0;

    ptr = sf_realloc(NULL, 0);
    cr_assert(ptr == NULL, "Should've returned null");
    cr_assert(errno != 0, "errno not set to the right value");
}

Test(sf_memsuite, Free_bounds_check, .init = sf_mem_init, .fini = sf_mem_fini){
    errno = 0;
    sf_free(NULL);

    cr_assert(errno == 0, "Not supposed to ste errno when pasing null");
    void* some_ptr = sf_malloc(16);

    // lower bound check
    sf_free(some_ptr-8);
    cr_assert(errno != 0, "errno not set to the right value");
    errno = 0;

    // upper bound check
    sf_free(some_ptr + 4096 + 8);
    cr_assert(errno != 0, "errno not set to the right value");
    errno = 0;

    void* new_ptr = sf_realloc(some_ptr-8, 78);
    cr_assert(new_ptr == NULL, "Should've returned null");
    cr_assert(errno != 0, "errno not set to the right value");
    errno = 0;

    new_ptr = sf_realloc(some_ptr + 4096 + 8, 78);
    cr_assert(new_ptr == NULL, "Should've returned null");
    cr_assert(errno != 0, "errno not set to the right value");

}

Test(sf_memsuite, Crazy_bounds_check, .init = sf_mem_init, .fini = sf_mem_fini){
    void* most = sf_malloc(4048);
    void* some_ptr = sf_malloc(16);
    cr_assert(freelist_head == NULL);
    errno = 0;

    void* test_ptr = sf_realloc(some_ptr + 1, 8);
    cr_assert(test_ptr == NULL, "Should've returned null");
    cr_assert(errno != 0, "errno not set to the right value");
    errno = 0;

    sf_free(some_ptr + 1);
    cr_assert(errno != 0, "errno not set to the right value");

    test_ptr = sf_realloc(some_ptr, 80);
    cr_assert(SHIFT_BACK(freelist_head->next->header.block_size) == 4000);
    sf_snapshot(true);
    sf_free(most);

}

Test(sf_memsuite, Padding_check, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr = sf_malloc(32);
    sf_header* header = ptr -8;
    int first_size = SHIFT_BACK(header->block_size);
    void* ptr2 = sf_realloc(ptr, 28);
    header = ptr2 -8;
    int second_size = SHIFT_BACK(header->block_size);

    cr_assert(ptr = ptr2, "Address not equal.");
    cr_assert(first_size == second_size, "block sizes not equal");
    printf("%d\n", header->padding_size);
    cr_assert(header->padding_size == 4);

}

Test(sf_memsuite, Shinking_merging_check, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr1 = sf_malloc(32);
    void* ptr2 = sf_malloc(32);
    void* ptr3 = sf_malloc(32);

    sf_free(ptr2);
    cr_assert(SHIFT_BACK(freelist_head->next->header.block_size) == 3952);
    void* new_ptr = sf_realloc(ptr1, 75);
    cr_assert(ptr1 == new_ptr);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 3952);
    sf_header* header = new_ptr -8;
    cr_assert(header->padding_size == 5);
    cr_assert(SHIFT_BACK(header->block_size) == 96);
    sf_free(ptr1);
    sf_free(ptr3);

    ptr1 = sf_malloc(40);
    ptr2 = sf_malloc(40);
    ptr3 = sf_malloc(40);
    header = ptr1 -8;
    cr_assert(header->padding_size == 8);
    sf_free(ptr2);
    new_ptr = sf_realloc(ptr1, 72);
    header = ptr1 -8;
    printf("%d\n", SHIFT_BACK(freelist_head->header.block_size));
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 32);
    cr_assert(header->padding_size == 8);
    cr_assert(SHIFT_BACK(header->block_size) == 96);

}

Test(sf_memsuite, Realloc_madness, .init = sf_mem_init, .fini = sf_mem_fini){
    int current_free_block_payload = 16368;
    int decrease_amount = 32;
    void* ptr1 = sf_malloc(current_free_block_payload);
    void* temp;
    cr_assert(freelist_head ==NULL);


    for (int i = 0; i < (16368/32); i++){
        current_free_block_payload -= decrease_amount;
        temp = sf_realloc(ptr1,current_free_block_payload);
        cr_assert(ptr1 == temp);
        cr_assert(SHIFT_BACK(freelist_head->header.block_size) == (decrease_amount * (i+1)));
        cr_assert(freelist_head->next == NULL);

    }

    printf("%d\n", SHIFT_BACK(freelist_head->header.block_size));
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 16384 - 32);



}

Test(sf_memsuite, already_freed, .init = sf_mem_init, .fini = sf_mem_fini){
    void* ptr1 = sf_malloc(32);
    sf_header* header = ptr1 - 8;
    sf_free(ptr1);
    sf_footer* footer = GO_TO_FOOTER((void*)header , 48);
    cr_assert(header->alloc == 0 && header->padding_size == 0);
    cr_assert(footer->alloc == 0);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 4096);
    errno = 0;

    sf_free(ptr1); // calling free again does nothing
    cr_assert(header->alloc == 0 && header->padding_size == 0);
    cr_assert(footer->alloc == 0);
    cr_assert(errno == 0);
    cr_assert(SHIFT_BACK(freelist_head->header.block_size) == 4096);

}
