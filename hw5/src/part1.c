#include "lott.h"

#define FILENAME_SIZE 256
#define LINE_SIZE 48
#define COUNTRYCODE_SIZE 2
#define TIMESTAMP_SIZE 10

typedef struct {
    void *info;
    sinfo *next;
} sinfo;

typedef struct {
    char code[COUNTRYCODE_SIZE];
    int count;
    count_node *next; 
} count_node;

static void* map(void*);
static void* reduce(void*);

int part1() {
    // Find all files in data directory and store in array
    DIR *dirp;
    struct dirent *direp;
    FILE *curfile;
    size_t nfiles = 1, filenames_size = 1;
    char **filenames = malloc(filenames_size, sizeof(char*));

    // Expand array of pointers dynamically
    for (filenames_size = 0; (direp = readdir(dirp)) != NULL; ++nfiles) {
        // Expand array to twice its size
        if (filenames_size < nfiles) {
            filenames_size *= 2;
            filenames = realloc(filenames_size * sizeof(char*), filenames);
        }
        // Assign new pointer to buffer of size FILENAME_SIZE
        filenames[nfiles - 1] = malloc(FILENAME_SIZE, sizeof(char));
        strcpy(filenames[nfiles - 1], direp->d_name);
    }

    // Spawn a thread for each file found and store in array
    pthread_t t_readers[nfiles];
    // Spawn a thread for this file
    for (int i = 0; i < nfiles; ++i) {
        file = fopen(filenames[i], O_RDONLY);
        pthread_create(&t_readers[i], NULL, map, file);
        close(filenames[i]);
    }
    printf(
        "Part: %s\n"
        "Query: %s\n",
        PART_STRINGS[current_part], QUERY_STRINGS[current_query]);

    return 0;
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
        num += str[i] - '0'
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
void find_avg_dur(FILE *file, sinfo *info) {
    char line[LINE_SIZE], **linep = &line, *durstr;
    int nvisits = 0, duration = 0;
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, file) != NULL) {
        // Find duration segment of line
        strsep(linep, ",");
        durstr = strsep(linep, ",");
        // Add duration to total
        duration += stoi(durstr, strlen(durstr));
        ++nvisits;
    } 

    // Find average duration
    info->info = malloc(sizeof(double));
    *(info->info) = (double)duration / (double)nvisits;
}

// Binary searches for year in used_years
int check_year_used(int year, int *used_years, int nyears) {
    // Odd length array - check last index and treat as array of nyears - 1
    if (nyears-- % 2) {
        if (used_years[nyears] == year)
            return 1;
    }

    // Binary search even length array
    int i = nyears >> 1 , j = nyears >> 2;
    while (year != used_years[i]) {
        if (year < used_years[i]) {
            i += j;
        } else {
            i -= j;
        }
        j >>= 1;
        // No match found
        if (j == 0) {
            return 0;
        }
    }
    return 1;
}

/**
* (C/D) Map function for finding average users per year, sets average
* in passed sinfo node
*
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
void find_avg_user(FILE *file, sinfo *info) {
    char line[LINE_SIZE], **linep = &line, *timestamp;
    int nvisits = 0, nyears = 0;
    
    // For all lines in file
    while (fgets(line, LINE_SIZE, file) != NULL) {
        // Find timestamp segment of line
        timestamp = strsep(linep, ",");

        // Find year from timestamp
        time_t ts = stol(timestamp, TIMESTAMP_SIZE);
        struct tm *tm = localtime(ts);
    } 

    // Find average duration
    info->info = malloc(sizeof(double));
    *(info->info) = (double)nvisits / (double)nyears;
}

/**
* (E) Map function for finding country count, creates linked list for all 
* countries 
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store country counts list in
*/
void find_max_country(FILE *file, sinfo *info) {
    char line[LINE_SIZE], **linep = &line;
    int nvisits = 0, found = 0;
    
    info->info = malloc(sizeof(count_node));
    count_node *head = NULL, *cursor = head;
    // For all lines in file
    while (fgets(line, LINE_SIZE, file) != NULL) {
        // Find country code segment of line
        strsep(linep, ","), strsep(linep, ","), strsep(linep, ",");;
        
        if (cursor != NULL) 
        while (cursor == NULL && strncmp(linep, cursor->code) != 0) {
            cursor = cursor->next;    
            if  {
                found = 0;
                break;   
            }
        }

        if (!found) {
            cursor = head;
            cursor 
        }
    } 
}
  
static void* map(void* v) {
    FILE *file = v;
    
    // Find map for current query
    void* (f_map*)(FILE*, sinfo*);
    switch (current_query) {
        case A:
        case B:
            f_map = find_avg_dur;
            break;
        case C:
        case D:
            f_map = find_avg_user;
            break;
        case E:
            f_map = find_max_country;
    }

    // Call map for query
    sinfo info;
    (*f_map)(file, &info);

    return NULL;
}

static void* reduce(void* v) {
    // FREE ALL info POINTERS!!!!!!!!!!
    while (cursor != NULL) {
        // Add to total
        total += *(cursor->info)
        // Restore resources
        free(cursor->info);
    }
    return NULL;
}
