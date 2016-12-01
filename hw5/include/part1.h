#include <time.h>

#define FILENAME_SIZE 256
#define LINE_SIZE 48
#define CCOUNT_SIZE 675
#define TIMESTAMP_SIZE 10

typedef struct sinfo {
    char filename[FILENAME_SIZE];
    double average;
    unsigned short *einfo;
    struct sinfo *next;
} sinfo;

/**
* Map controller, calls map function for current query,
* Acts as start routine for created threads 
*
* @param v Pointer to input file
* @return Pointer to sinfo struct
*/
static void* map(void* v);

/**
* (A/B) Map function for finding average duration of visit, sets average in 
* passed sinfo node
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
void map_avg_dur(FILE *file, sinfo *info);

/**
* (C/D) Map function for finding average users per year, sets average
* in passed sinfo node
*
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
void map_avg_user(FILE *file, sinfo *info);

/**
* (E) Map function for finding country count, creates linked list for all 
* countries 
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store country counts list in
*/
void map_max_country(FILE *file, sinfo *info);

/**
* Reduce controller, calls reduce function for current query
* 
* @param v Pointer to head of sinfo linked list
* @return Pointer to sinfo containing result
*/
static void* reduce(void* v);

/**
* (A/B/C/D) Reduce function for finding max/min average in sinfo
* linked list, bases result from current_query 
*
* @param head Pointer to head of sinfo linked list
* @return Pointer to sinfo node with max/min average 
*/
void *reduce_avg(sinfo *head);

/**
* (E) Reduce function for finding country with the most users
*
* @param head Pointer to head of sinfo linked list
* @return Pointer todo
*/
void *reduce_max_country(sinfo *head);