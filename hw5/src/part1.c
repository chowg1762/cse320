#include "lott.h"
#include "part1.h"

int part1() {
    // Find all files in data directory and store in array
    DIR *dirp = opendir(DATA_DIR);
    struct dirent *direp;
    FILE *file;
    size_t nfiles;

    // Create linked list of sinfo nodes, nfiles long
    sinfo head, *cursor = &head;
    for (nfiles = 1; (direp = readdir(dirp)) != NULL; ++nfiles) {
        if (nfiles == 1) {
            strcpy(head.filename, direp->d_name);
            continue;
        }
        sinfo new_node;
        strcpy(new_node.filename, direp->d_name);
        if (current_query == E) {
            unsigned short ccount[CCOUNT_SIZE];
            new_node.einfo = ccount;
        }
        
        cursor->next = &new_node;
        cursor = cursor->next;
    }

    // Spawn a thread for each file found and store in array
    pthread_t t_readers[nfiles];
    cursor = &head;
    for (int i = 0; i < nfiles; ++i) {
        file = fopen(cursor->filename, "r");
        pthread_create(&t_readers[i], NULL, map, file);
    }

    reduce(&head);

    printf(
        "Part: %s\n"
        "Query: %s\n",
        PART_STRINGS[current_part], QUERY_STRINGS[current_query]);

    return 0;
}

static void* map(void* v) {
    FILE *file = v;
    
    // Find map for current query
    void (*f_map)(FILE*, sinfo*);
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
    sinfo info;
    (*f_map)(file, &info);

    fclose(file);

    return NULL;
}

// Converts string to integer
int stoi(char *str, int n) {
    int num = 0;
    for (int i = 0; i < n; ++i) {
        num *= 10;
        num += str[i] - '0';
    }
    return num;
}

// Converts string to long
long stol(char *str, int n) {
    long num = 0;
    for (int i = 0; i < n; ++i) {
        num *= 10;
        num += str[i] - '0';
    }
    return num;
}

/**
* (A/B) Map function for finding average duration of visit, sets average in 
* passed sinfo node
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
void map_avg_dur(FILE *file, sinfo *info) {
    char line[LINE_SIZE], *linep = line, *durstr;
    int nvisits = 0, duration = 0;
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, file) != NULL) {
        // Find duration segment of line
        strsep(&linep, ",");
        durstr = strsep(&linep, ",");
        // Add duration to total
        duration += stoi(durstr, strlen(durstr));
        ++nvisits;
    } 

    // Find average duration 
    info->average = (double)duration / nvisits;
}

// Helper for map_avg_user
// Checks bit array used_years for previously found years
// Returns 1 if year not found, else 0
int check_year_used(int year, long *used_years, int nyears) {
    year -= 70;
    
    // Check bit at offset from 1970
    long mask = 1;
    mask <<= year;
    mask &= *used_years;
    if (mask & *used_years) {
        // Match found
        return 0;
    }
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
void map_avg_user(FILE *file, sinfo *info) {
    char line[LINE_SIZE], *linep = line, *timestamp;
    int nvisits = 0, nyears = 0;
    long used_years = 0;
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, file) != NULL) {
        // Find timestamp segment of line
        timestamp = strsep(&linep, ",");

        // Find year from timestamp
        time_t ts = stol(timestamp, TIMESTAMP_SIZE);
        struct tm *tm = localtime(&ts);
        nyears += check_year_used(tm->tm_year, &used_years, nyears);
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
void map_max_country(FILE *file, sinfo *info) {
    char line[LINE_SIZE], *linep = line;
    int ind;

    memset(info->einfo, 0, CCOUNT_SIZE);
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, file) != NULL) {
        // Find country code segment of line
        strsep(&linep, ","), strsep(&linep, ","), strsep(&linep, ",");
         
        // Turn country_code into an index
        ind = ((linep[0] - 'A') * 26) + (linep[1] - 'A');

        // Add to count for that country
        ++(info->einfo[ind]);
    } 
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

// Helper for reduce_avg_dur and reduce_avg_user
// Returns comparison based on current_query
char durcmp(sinfo *a, sinfo *b) {
    if (current_query == A || current_query == C) {
        if (a->average > b->average) {
            return 1;
        } else if (a->average < b->average) {
            return -1;
        } else {
            return 0;
        }
    } else {
        if (a->average < b->average) {
            return 1;
        } else if (a->average > b->average) {
            return -1;
        } else {
            return 0;
        }
    }
}

/**
* (A/B/C/D) Reduce function for finding max/min average in sinfo
* linked list, bases result from current_query 
*
* @param head Pointer to head of sinfo linked list
* @return Pointer to sinfo node with max/min average
*/
void *reduce_avg(sinfo *head) {
    sinfo *cursor = head->next, *result = head;
    char res;
    
    // Handle trivial cases
    if (head->next == NULL) {
        return head;
    }

    // Find query result
    while (cursor != NULL) {
        res = durcmp(cursor, result);
        if (res > 0) {
            result = cursor;
        } 
        // Equal - pick alphabetical order first
        else if (res == 0) {
            if (strcmp(cursor->filename, result->filename) < 0) {
                result = cursor;
            }
        }
        cursor = cursor->next;
    }

    return result;
}

/**
* (E) Reduce function for finding country with the most users
*
* @param head Pointer to head of sinfo linked list
* @return Pointer to sinfo node with highest country user count
*/
void *reduce_max_country(sinfo *head) {
    sinfo *max = NULL, *cursor = head;
  
    while (cursor != NULL) {
        // Find index of max country user count
        int maxind = 0;
        for (int i = 1; i < CCOUNT_SIZE; ++i) {
            if (cursor->einfo[maxind] < cursor->einfo[i]) {
                maxind = i;
            }
        }
        cursor->average = maxind;

        // Set new max if needed
        if (max == NULL || 
        cursor->einfo[(int)cursor->average] > max->einfo[(int)max->average]) {
            max = cursor;
        } 
        else if (cursor->einfo[(int)cursor->average] == 
        max->einfo[(int)max->average]) {
            // Equal - pick alphabetical order first
            if (strcmp(cursor->filename, max->filename) > 0) {
                max = cursor;
            }
        }

        cursor = cursor->next;
    }
    
    return max;
}
