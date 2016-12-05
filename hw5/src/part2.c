#include "lott.h"
#include "parts1_2.h"

int part2(size_t nthreads) {
    // Find all files in data directory and store in array
    DIR *dirp = opendir(DATA_DIR);
    struct dirent *direp;
    size_t nfiles;

    // Create linked list of sinfo nodes, nfiles long
    sinfo *head = calloc(1, sizeof(sinfo)), *cursor = head;
    for (nfiles = 0; (direp = readdir(dirp)) != NULL; ++nfiles) {
        if (direp->d_name[0] == '.') {
            --nfiles;
            continue;
        }
        if (nfiles == 1) {
            strcpy(head->filename, direp->d_name);
            if (current_query == E) {
                head->einfo = calloc(CCOUNT_SIZE, sizeof(int));
            }
            continue;
        }
        sinfo *new_node = calloc(1, sizeof(sinfo));
        strcpy(new_node->filename, direp->d_name);
        if (current_query == E) {
            new_node->einfo = calloc(CCOUNT_SIZE, sizeof(int));
        }
        
        cursor->next = new_node;
        cursor = new_node;
    }

    pthread_t t_readers[nthreads];
    size_t rem_files = nfiles;
    int *t_return;
    char rel_filepath[FILENAME_SIZE];
    strcpy(rel_filepath, "./");
    strcpy(rel_filepath + 2, DATA_DIR);
    strcpy(rel_filepath + 6, "/");
    sinfo *marker;
    cursor = head;
    while (rem_files > 0) {
        // Spawn threads: nthreads or rem_files
        marker = cursor;
        for (int i = 0; i < rem_files && i < nthreads; ++i) {
            strcpy(rel_filepath + 7, cursor->filename);
            cursor->file = fopen(rel_filepath, "r");
            if (cursor->file == NULL) {
                exit(EXIT_FAILURE);
            }

            pthread_create(&t_readers[i], NULL, map, cursor);
            cursor = cursor->next;
        }

        // Join all used threads
        cursor = marker;
        for (int i = 0; i < rem_files && i < nthreads; ++i) {
            pthread_join(t_readers[i], &(cursor->t_return));
            cursor = cursor->next;
        }

        rem_files -= nthreads;
    }

    // Find result of query
    head = reduce(head);
    printf(
        "Part: %s\n"
        "Query: %s\n"
        "Result: %lf, %s\n",
        PART_STRINGS[current_part], QUERY_STRINGS[current_query],
        head->average, head->filename);

    // Restore resources
    sinfo *prev;
    cursor = head;
    if (current_query == E) {
        while (cursor != NULL) {
            free(cursor->einfo);     
            prev = cursor;
            cursor = cursor->next;
            free(prev);
        }
    } else {
        while (cursor != NULL) {
            prev = cursor;
            cursor = cursor->next;
            free(prev);
        }
    }

    return 0;
}

/**
* Map controller, calls map function for current query,
* Acts as start routine for created threads 
*
* @param v Pointer to input file
* @return Pointer to sinfo struct
*/
static void* map(void* v) {
    sinfo *info = v;
    
    // Find map for current query
    void (*f_map)(sinfo*);
    switch (current_query) {
        case A:
        case B:
            f_map = &map_avg_dur;
            break;
        case C:
        case D:
            f_map = &map_avg_user;
            break;
        case E:
            f_map = &map_max_country;
    }

    // Call map for query
    (*f_map)(info);

    fclose(info->file);
    pthread_exit(0);
    
    return NULL;
}

/**
* Reduce controller, calls reduce function for current query
* 
* @param v Pointer to head of sinfo linked list
* @return Pointer to sinfo containing result
*/
static void* reduce(void* v) {
    sinfo *head = v;

    // Find reduce for current query
    void* (*f_reduce)(sinfo*);
    if (current_query == E) {
        f_reduce = &reduce_max_country;
    } else {
        f_reduce = &reduce_avg;
    }

    return (*f_reduce)(head);
}