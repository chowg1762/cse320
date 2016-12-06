#ifndef PART3_H
#define PART3_H

#include <time.h>
#include <semaphore.h>

#define MR_FILENAME "mapred.tmp"
#define FILENAME_SIZE 256
#define LINE_SIZE 48
#define CCOUNT_SIZE 675
#define TIMESTAMP_SIZE 9

extern sem_t mut_pdcr, mut_buf;

typedef struct sinfo {
    FILE *file;
    char filename[FILENAME_SIZE];
    double average;
    unsigned int *einfo;
    void *t_return;
    struct sinfo *next;
} sinfo;

extern sinfo *buf_head, *buf_cursor;

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
static void map_avg_dur(sinfo *info);

/**
* (C/D) Map function for finding average users per year, sets average
* in passed sinfo node
*
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store average in 
*/
static void map_avg_user(sinfo *info);

/**
* (E) Map function for finding country count, creates linked list for all 
* countries 
* 
* @param file Pointer to open website csv file
* @param info Pointer to sinfo node to store country counts list in
*/
static void map_max_country(sinfo *info);

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
static void reduce_avg(sinfo *head);

/**
* (E) Reduce function for finding country with the most users
*
* @param head Pointer to head of sinfo linked list
* @return Pointer todo
*/
static void reduce_max_country(sinfo *head);

#endif