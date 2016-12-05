#include "lott.h"
#include "part3.h"

sem_t sem_fileacc;
FILE *mrfile;

int part3(size_t nthreads) {
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

    // Create mapred.tmp for mapping and reducing communication
    mrfile = fopen(MR_FILENAME, "w+");

    // Spawn reduce thread
    pthread_t t_reduce;
    sinfo result;
    pthread_create(&t_reduce, NULL, reduce, &result);

    // For all files use an available map thread, joining it after completion
    pthread_t t_maps[nthreads];
    rem_files = nfiles;
    char rel_filepath[FILENAME_SIZE];
    strcpy(rel_filepath, "./");
    strcpy(rel_filepath + 2, DATA_DIR);
    strcpy(rel_filepath + 6, "/");
    sinfo *marker;
    cursor = head;
    while (nrem > 0) {
        // Spawn threads: nthreads or rem_files
        marker = cursor;
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
        cursor = marker;
        for (int i = 0; i < rem_files && i < nthreads; ++i) {
            pthread_join(t_maps[i], &(cursor->t_return));
            cursor = cursor->next;
        }

        rem_files -= nthreads;
    }

    // Cancel
    pthread_cancel(t_reduce);

    // Find result of query
    sinfo result;
    head = reduce(&result);

    // Close and delete mapred.tmp file
    fclose(mrfile);
    unlink(MR_FILENAME);

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

// Semaphore locking wrapper for fprintf to mapred.tmp
static void s_writeinfo(sinfo *info) {
    sem_wait(&sem_fileacc);
    if (current_query != E) {
        fprintf(mrfile, "%s,%lf\n", info->filename, info->average);
    } else {
        fprintf(mrfile, "%s,%d,%d\n", info->filename, 
        info->einfo[(int)info->average], (int)info->average);
    }
    sem_post(&sem_fileacc);
}

// Semaphore locking wrapper for fgets from mapred.tmp
static char* s_fgets(char *buffer, int size) {
    char *r;
    sem_wait(&sem_fileacc);
    r = fgets(buffer, size, mrfile);
    sem_post(&sem_fileacc);
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
    (*f_map)(info);

    // Write query info to mapred.tmp
    s_writeinfo(info);

    fclose(info->file);
    pthread_exit(0);
    
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
}

/**
* Cancel routine for the reduce thread, prints results of query
*
* @param arg
*
*/
static void reduce_cancel(void *v) {

}

/**
* Reduce controller, calls reduce function for current query
* 
* @param v Pointer to head of sinfo linked list
* @return Pointer to sinfo containing result
*/
static void* reduce(void *v) {
    pthread_cleanup_push(&reduce_cancel, v);

    // Find reduce for current query
    void* (*f_reduce)(sinfo*);
    if (current_query == E) {
        f_reduce = &reduce_max_country;
    } else {
        f_reduce = &reduce_avg;
    }

    sinfo *result = v;
    return (*f_reduce)(result);
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
* @param nrem Number of remaining files to read info about
* @return Pointer to sinfo containing result
*/
static void *reduce_avg(sinfo *result) {
    char res;
    double avg;
    result->average = -1;

    // Find query result
    char line[LINE_SIZE], *linep;
    memset(line, 0, LINE_SIZE);
    while (1) {
        // Read available info from mapred.tmp 
        while (s_fgets(line, LINE_SIZE) != NULL) {
            // Block canceling since there is an entry
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            sscanf(line, "%*s,%lf,", &avg);
            res = avgcmp(avg, head->average);
            if (res > 0) {
                result->average = avg;
            } 
            // Equal - pick alphabetical order first
            else if (res == 0) {
                if (strcmp(cursor->filename, result->filename) < 0) {
                    result = cursor;
                }
            }
            linep = line;
        }
        // Enable canceling 
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)
    }

    return result;
}

/**
* (E) Reduce function for finding country with the most users
*
* @param result Pointer of sinfo to store result in 
* @param nrem Number of remaining files to read info about
* @return Pointer to sinfo node with highest country user count
*/
static void *reduce_max_country(sinfo *result) {
    unsigned int ccount[CCOUNT_SIZE], maxind;
    memset(ccount, 0, CCOUNT_SIZE * sizeof(int));
    
    sinfo *cursor = head;
    while (cursor != NULL) {
        // Find index of max country user count
        maxind = 0;
        for (int i = 1; i < CCOUNT_SIZE; ++i) {
            // If i > maxind element OR i == maxind element and lex lesser
            if (cursor->einfo[i] > cursor->einfo[maxind]) {
                maxind = i;
            }
        }

        // Add max country to combined list
        ccount[maxind] += cursor->einfo[maxind];

        cursor = cursor->next;
    }

    // Find max country user count in combined list
    maxind = 0;
    for (int i = 1; i < CCOUNT_SIZE; ++i) {
        if (ccount[i] > ccount[maxind]) {
            maxind = i;
        }
    }

    head->filename[0] = (maxind / 26) + 'A';
    head->filename[1] = (maxind % 26) + 'A';
    head->filename[2] = '\0';
    head->average = ccount[maxind];

    return head;
}
