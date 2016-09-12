//**DO NOT** CHANGE THE PROTOTYPES FOR THE FUNCTIONS GIVEN TO YOU. WE TEST EACH
//FUNCTION INDEPENDENTLY WITH OUR OWN MAIN PROGRAM.
#include "map_reduce.h"

void printhelp() {
    printf("Usage:  ./mapreduce [h|v] FUNC DIR\n\
    \tFUNC    Which operation you would like to run on the data:\n\
    \t\tana - Analysis of various text files in a directory.\n\
    \t\tstats - Calculates stats on files which contain only numbers.\n\
    \tDIR     The directory in which the files are located.\n\n\
    \tOptions:\n\
    \t-h      Prints this help menu.\n\
    \t-v      Prints the map function’s results, stating the file it’s from.\n");
}

int validateargs(int argc, char **argv) {
    // No args given
    if (argc == 1) {
        printhelp();
        return -1;
    }
    // Print menu
    if (strcmp("-h", argv[1]) == 0) {
        printhelp();
        return 0;
    }
    // Optional flag
    int opFlagSel = 0, i = 1;
    if (argc == 4) {
        if (strcmp("-v", argv[i++]) == 0) {
            opFlagSel = 1;
        } 
    }
    // Analyis
    if (strcmp("ana", argv[i]) == 0) {
        return 1 + opFlagSel * 2;
    } 
    // Stats
    else if (strcmp("stats", argv[i]) == 0) {
        return 2 + opFlagSel * 2;
    }
    // Invalid
    printhelp(); 
    return -1;
}

int nfiles(char *dir) {
    DIR* dirStream = opendir(dir);
    // Error accessing directory
    if (dirStream == NULL) {
        closedir(dirStream);
        return -1;
    }
    // Count files in directory
    int n = 0;
    struct dirent *dirPtr; 
    while ((dirPtr = readdir(dirStream)) != NULL) {
        if (strcmp(dirPtr->d_name, ".") != 0 && 
        strcmp(dirPtr->d_name, "..") != 0) {
            ++n;
        }
    }
    if (closedir(dirStream) == -1)
        return -1;
    return n;
}

int map(char *dir, void *results, size_t size, 
    int (*act)(FILE *f, void *res, char *fn)) {
    DIR* dirStream = opendir(dir);
    // Error accessing directory
    if (dirStream == NULL) {
        closedir(dirStream);
        return -1;
    }
    int sum = 0, i, j;
    struct dirent *dirPtr;
    while ((dirPtr = readdir(dirStream)) != NULL) {
        if (strcmp(dirPtr->d_name, ".") != 0 && 
        strcmp(dirPtr->d_name, "..") != 0) {
            // Get relative path of file
            int pathLength = strlen(dir) + strlen(dirPtr->d_name) + 2;
            char dirPath[pathLength];
            for (i = 0; i < strlen(dir); ++i) {
                dirPath[i] = dir[i];
            }
            dirPath[i++] = '/';
            for (j = 0; j < strlen(dirPtr->d_name); ++j) {
                dirPath[i++] = dirPtr->d_name[j];
            }
            dirPath[pathLength - 1] = '\0';
            // Open file
            FILE *filePtr;
            if ((filePtr = fopen(dirPath, "r")) == NULL) {
                closedir(dirStream);
                return -1;
            }
            // Perform some action and store result
            sum += (*act)(filePtr, results, dirPtr->d_name); // HERE
            results += size;
            // Close file
            fclose(filePtr);
        }
    }
    return sum;
}

struct Analysis analysis_reduce(int n, void *results) {
    struct Analysis reducedAna;
    struct Analysis *resultsStructs = results;
    memset(&reducedAna, 0, sizeof(reducedAna));
    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < 128; ++j) {
            reducedAna.ascii[j] += resultsStructs[i].ascii[j];
        }
        if (resultsStructs[i].lnlen > reducedAna.lnlen) {
            memcpy(reducedAna.filename, resultsStructs[i].filename, sizeof);
            reducedAna.lnlen = resultsStructs[i].lnlen; 
            reducedAna.lnno = resultsStructs[i].lnno;
        }
    }
    printf("%s\n", reducedAna.filename);
    return reducedAna;
}

Stats stats_reduce(int n, void *results) {
    Stats reducedSta;
    Stats *resultsStructs = results;
    memset(&reducedSta, 0, sizeof(reducedSta));
    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < NVAL; ++j) {
            reducedSta.histogram[j] += resultsStructs[i].histogram[j];
        }
        reducedSta.sum += resultsStructs[i].sum;
        reducedSta.n += resultsStructs[i].n;
    }
    return reducedSta;
}

void analysis_print(struct Analysis res, int nbytes, int hist) {
    printf("FN(%lx) LNL(%lx)", (long)&res.filename, (long)&res.lnlen);
    printf("File: %s\n", res.filename);
    printf("Longest line length: %d\n", res.lnlen);
    printf("Longest line number: %d\n", res.lnno);
    printf("Total bytes in directory: %d\n", nbytes);
    if (hist == 0)
        return;
    printf("Histogram:\n");
    int i, j;
    for (i = 0; i < 128; ++i) {
        if (res.ascii[i] > 0) {
            printf(" %d:", i);
            for (j = 0; j < res.ascii[i]; ++j) {
                printf("-");
            }
            printf("\n");
        }
    }
}

void stats_print(Stats res, int hist) {
    int i, j, max = 0, count = 0; 
    float q1 = 0, q3 = 0, median = 0;
    if (hist != 0) {
        printf("Histogram:\n");
        for (i = 0; i < NVAL; ++i) {
            if (res.histogram[i] > 0) {
                printf("%d  :", i);
                for (j = 0; j < res.histogram[i]; ++j) {
                    printf("-");
                }
                printf("\n");
            }
        }
        printf("\n");
    }
    printf("Count: %d\n", res.n);
    printf("Mean: %f\n", res.sum / (float)res.n);
    printf("Mode: ");
    for (i = 0; i < NVAL; ++i) {
        if (res.histogram[i] > max) {
            max = res.histogram[i];
            count = 0;
        }
        else if (res.histogram[i] == max) {
            ++count;
        }
    }
    for (i = 0; i < NVAL; ++i) {
        if (res.histogram[i] == max) {
            printf("%d", i);
            if (count-- > 0) {
                printf(" ");
            }
        }
    }
    printf("\n");
    i = 0;
    count = res.n;
    while (count > 0) {
        count -= res.histogram[i++]; 
        if (count < (res.n * 0.75)) {
            q1 = i - 1;
            break;
        }
    }
    i = 0;
    count = res.n;
    while (count > 0) {
        count -= res.histogram[i++]; 
        if (count < (res.n / 2)) {
            median = i - 1;
            break;
        }
    }
    i = 0;
    count = res.n;
    while (count > 0) {
        count -= res.histogram[i++]; 
        if (count < (res.n * 0.25)) {
            q3 = i - 1;
            break;
        }
    }
    printf("Median: %f\n", median);
    printf("Q1: %f\n", q1);
    printf("Q3: %f\n", q3);
    max = 0;
    for (i = 0; i < NVAL; ++i) {
        if (res.histogram[i] > 0) {
            printf("Min: %d\n", i);
            break;
        }
    }
    for (i = NVAL - 1; i >= 0; --i) {
        if (res.histogram[i] > 0) {
            printf("Max: %d\n", i);
            break;
        }
    }
}

int analysis(FILE *f, void *res, char *filename) {
    struct Analysis *newAnalysis = res;
    int i;
    for (i = 0; i < strlen(filename); ++i) {
        newAnalysis->filename[i] = filename[i];
    }
    newAnalysis->filename[++i] = '\0';
    newAnalysis->lnlen = 0;
    for (i = 0; i < 128; ++i) {
        newAnalysis->ascii[i] = 0;
    }
    int byteCount = 0, lineLength = 0, lineCount = 1, currChar;
    while ((currChar = fgetc(f)) != EOF) {
        if (currChar == 10) { // \n found
            if (lineLength > newAnalysis->lnlen) {
                newAnalysis->lnlen = lineLength;
                newAnalysis->lnno = lineCount;
            }
            ++lineCount;
            lineLength = 0;
        } else {
            ++lineLength;
        }
        ++newAnalysis->ascii[currChar];
        ++byteCount;
    }
    return byteCount;
}

int stats(FILE *f, void *res, char *filename) {
    Stats *newStats = res;
    int i;
    for (i = 0; i < strlen(filename); ++i) {
        newStats->filename[i] = filename[i];
    }
    newStats->filename[++i] = '\0';
    newStats->sum = 0;
    newStats->n = 0;
    for (i = 0; i < NVAL; ++i) {
        newStats->histogram[i] = 0;
    }
    int currNum;
    while (fscanf(f, "%d", &currNum) != EOF) {
        if (currNum < 0 || currNum > 31) {
            return -1;
        }
        ++newStats->histogram[currNum];
        newStats->sum += currNum;
        ++newStats->n;
    }
    return 0;
}