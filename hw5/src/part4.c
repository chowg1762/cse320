#include "lott.h"
#include "part4.h"

sem_t mut_pdcr, mut_buf;
int nproducers;
sinfo *buf_head;

int part4(size_t nthreads) {

    // Initialize semaphores
    sem_init(&mut_pdcr, 0, 1), sem_init(&mut_buf, 0, 1);

    // Find all files in data directory and store in array
    DIR *dirp = opendir(DATA_DIR);
    struct dirent *direp;
    size_t nfiles, rem_files;

    // Create linked list of sinfo nodes, nfiles long
    sinfo *head = calloc(1, sizeof(sinfo)), *cursor = head;
    for (nfiles = 0; (direp = readdir(dirp)) != NULL; ++nfiles) {
        if (direp->d_name[0] == '.') {
            --nfiles;
            continue;
        }
        if (nfiles == 0) {
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
    closedir(dirp);

    // Spawn reduce thread
    pthread_t t_reduce;
    sinfo result;
    memset(&result, 0, sizeof(sinfo));
    if (current_query == E) {
        result.einfo = calloc(CCOUNT_SIZE, sizeof(int));
    }
    pthread_create(&t_reduce, NULL, reduce, &result);

    // For all files use an available map thread, joining it after completion
    pthread_t t_maps[nthreads];
    rem_files = nfiles;
    char rel_filepath[FILENAME_SIZE];
    strcpy(rel_filepath, "./");
    strcpy(rel_filepath + 2, DATA_DIR);
    strcpy(rel_filepath + 6, "/");
    cursor = head;
    while (rem_files > 0) {
        // Spawn threads: nthreads or rem_files
        for (int i = 0; i < rem_files && i < nthreads; ++i) {
            strcpy(rel_filepath + 7, cursor->filename);
            cursor->file = fopen(rel_filepath, "r");
            if (cursor->file == NULL) {
                exit(EXIT_FAILURE);
            }

            pthread_create(&t_maps[i], NULL, map, cursor);
            cursor = cursor->next;
        }

        // Join all used threads
        for (int i = 0; i < rem_files && i < nthreads; ++i) {
            pthread_join(t_maps[i], NULL);
        }

        rem_files -= nthreads;
    }

    // Cancel reduce thread since all map threads have been joined
    pthread_cancel(t_reduce);
    pthread_join(t_reduce, NULL);

    // Restore resources
    sinfo *prev;
    cursor = buf_head;
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

// Converts string to integer
static int stoi(char *str, int n) {
    int num = 0;
    for (int i = 0; i < n; ++i) {
        num *= 10;
        num += str[i] - '0';
    }
    return num;
}

// Converts string to long
static long stol(char *str, int n) {
    long num = 0;
    for (int i = 0; i < n; ++i) {
        num *= 10;
        num += str[i] - '0';
    }
    return num;
}

// Semaphore locking storage access for global buffer list
static void s_storeinfo(sinfo *info) {
    // Wait to change nproducers
    sem_wait(&mut_pdcr);
    if (++nproducers == 1) {
        // Only producer here - wait for access lock
        sem_wait(&mut_buf);
    }
    // Let others change nproducers
    sem_post(&mut_pdcr);

    // Store results in global buffer list
    info->next = buf_head;
    buf_head = info;

    sem_wait(&mut_pdcr);
    if (--nproducers == 0) {
        // Only producer here - free access lock
        sem_post(&mut_buf);
    }
    sem_post(&mut_pdcr);
}

// Semaphore locking consumer access for global buffer list
static int s_consumeinfo(sinfo **info) {
    int r = 0;
    // Wait for all producers to free access lock
    sem_wait(&mut_buf);

    // Consume from global buffer list
    if ((*info = buf_head) != NULL) {
        buf_head = buf_head->next;
        r = 1;
    } else {
        r = 0;
    }

    // Free access lock 
    sem_post(&mut_buf);
    return r;
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
    // printf("Pre call: %s\n", info->filename);
    (*f_map)(info);
    // printf("Post call: %s\n", info->filename);

    // Store info to global buffer list
    s_storeinfo(info);

    fclose(info->file);
    
    pthread_exit(NULL);
    return NULL;
}

/**
* (A/B) Map function for finding average duration of visit, sets average in 
* passed sinfo node
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
static void map_avg_dur(sinfo *info) {
    char line[LINE_SIZE], *linep = line, *durstr;
    int nvisits = 0, duration = 0;
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, info->file) != NULL) {
        // Find duration segment of line
        strsep(&linep, ","), strsep(&linep, ",");
        durstr = strsep(&linep, ",");
        // Add duration to total
        duration += stoi(durstr, strlen(durstr));
        linep = line;
        ++nvisits;
    }

    // Write average duration 
    info->average = (double)duration / nvisits;
}

// Helper for map_avg_user
// Checks bit array used_years for previously found years
// Returns 1 if year not found, else 0
static int check_year_used(int year, unsigned long *used_years) {
    // Check bit at offset from 1970
    unsigned long mask = 1;
    year -= 70;
    mask <<= year;
    if (mask & *used_years) {
        // Match found - year used
        return 0;
    }
    // Match not found - year not used
    *used_years |= mask;
    return 1;
}

/**
* (C/D) Map function for finding average users per year, sets average
* in passed sinfo node
*
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
static void map_avg_user(sinfo *info) {
    char line[LINE_SIZE], *linep = line, *timestamp;
    int nvisits = 0, nyears = 0;
    unsigned long used_years = 0;
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, info->file) != NULL) {
        // Find timestamp segment of line
        timestamp = strsep(&linep, ",");

        // Find year from timestamp
        time_t ts = stol(timestamp, strlen(timestamp));
        struct tm *tm = localtime(&ts);

        // Add to nyears if year is new
        nyears += check_year_used(tm->tm_year, &used_years);
        ++nvisits;
        linep = line;
    } 

    // Find average users
    info->average = (double)nvisits / nyears;
}

/**
* (E) Map function for finding country count, creates linked list for all 
* countries 
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store country counts list in
*/
static void map_max_country(sinfo *info) {
    char line[LINE_SIZE], *linep = line;
    int ind;

    // For all lines in file
    while (fgets(line, LINE_SIZE, info->file) != NULL) {
        // Find country code segment of line
        strsep(&linep, ","), strsep(&linep, ","), strsep(&linep, ",");
         
        // Turn country_code into an index
        ind = ((linep[0] - 'A') * 26) + (linep[1] - 'A');

        // Add to count for that country
        ++(info->einfo[ind]);
        linep = line;
    }

    // Find max country count with lexicographical tie breaking
    for (int i = 0; i < CCOUNT_SIZE; ++i) {
        if (info->einfo[i] > info->einfo[ind]) {
            ind = i;
        }
    } 
    info->average = ind;
}

/**
* Cancel routine for the reduce thread, prints results of query
*
* @param v Pointer to sinfo containing results
*/
static void reduce_cancel(void *v) {
    sinfo *result = v;
    if (current_query == E) {
        int max = 0;
        for (int i = 1; i < CCOUNT_SIZE; ++i) {
            if (result->einfo[i] > result->einfo[max]) {
                max = i;
            }
        }

        result->filename[0] = (max / 26) + 'A';
        result->filename[1] = (max % 26) + 'A';
        result->filename[2] = '\0';
        result->average = result->einfo[max];
    }

    printf(
        "Part: %s\n"
        "Query: %s\n"
        "Result: %lf, %s\n",
        PART_STRINGS[current_part], QUERY_STRINGS[current_query],
        result->average, result->filename);
    fflush(NULL);
}

/**
* Reduce controller, calls reduce function for current query
* 
* @param v Pointer to head of sinfo linked list
* @return Pointer to sinfo containing result
*/
static void *reduce(void *v) {
    pthread_cleanup_push(&reduce_cancel, v);

    // Find reduce for current query
    void (*f_reduce)(sinfo*);
    if (current_query == E) {
        f_reduce = &reduce_max_country;
    } else {
        f_reduce = &reduce_avg;
    }

    // Find query result
    sinfo *result = v;
    (*f_reduce)(result);

     pthread_cleanup_pop(1);
     return NULL;
}

// Helper for reduce_avg
// Returns comparison based on current_query
static char avgcmp(double a, double b) {
    if (current_query == A || current_query == C) {
        if (a > b) {
            return 1;
        } else if (a < b) {
            return -1;
        } else {
            return 0;
        }
    } else {
        if (a < b) {
            return 1;
        } else if (a > b) {
            return -1;
        } else {
            return 0;
        }
    }
}

/**
* (A/B/C/D) Reduce function for finding max/min average in mapred.tmp, 
* bases result from current_query 
*
* @param result Pointer of sinfo to store result in
*/
static void reduce_avg(sinfo *result) {
    sinfo *cursor = NULL;
    char res;

    // Until cancelled
    while (1) {
        // Consume available info from global buffer list
        while (s_consumeinfo(&cursor)) {
            // Block canceling since there is an entry
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

            // Compare with current best
            res = avgcmp(cursor->average, result->average);
            if (res > 0) {
                result->average = cursor->average;
                strcpy(result->filename, cursor->filename);
            }
            // Equal - pick alphabetical order first
            else if (res == 0) {
                if (strcmp(cursor->filename, result->filename) < 0) {
                    result->average = cursor->average;
                    strcpy(result->filename, cursor->filename);
                }
            }
        }
        // Ran out of entries - enable canceling
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
}

/**
* (E) Reduce function for finding country with the most users
*
* @param result Pointer of sinfo to store result in
*/
static void reduce_max_country(sinfo *result) {
    sinfo *cursor = NULL;
    int *code = (int*)(&result->average);
    
    // Until cancelled
    while (1) {
        // Read line of file and add count to index code 
        while (s_consumeinfo(&cursor)) {

            // Block canceling since there is an entry
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  
            // Add count to country code
            result->einfo[*code] += cursor->einfo[*code]; 
        }
        // Enable canceling 
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
}
