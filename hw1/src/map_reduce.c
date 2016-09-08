//**DO NOT** CHANGE THE PROTOTYPES FOR THE FUNCTIONS GIVEN TO YOU. WE TEST EACH
//FUNCTION INDEPENDENTLY WITH OUR OWN MAIN PROGRAM.
#include "map_reduce.h"

void printhelp() {
    printf("Usage:  ./mapreduce [h|v] FUNC DIR\n
    \t\tFUNC    Which operation you would like to run on the data:\n
    \t\tana - Analysis of various text files in a directory.\n
    \t\tstats - Calculates stats on files which contain only numbers.\n
    \t\tDIR     The directory in which the files are located.\n
    \t\tOptions:\n
    \t\t-h      Prints this help menu.\n
    \t\t-v      Prints the map function’s results, stating the file it’s from.\n");
}

int validateargs(int argc, char **argv) {
    // No args given
    if (argc < 1)
        return -1;
    // Print menu
    if (strcmp("-h", argv[0]) == 0) {
        printhelp();
        return 0;
    }
    // Optional flag
    int opFlagSel = 0, i = 0;
    if (argc == 3) {
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
    return -1;
}

int nfiles(char *dir) {
    DIR* dirStream = opendir(dir);
    // Error accessing directory
    if (dirStream == NULL) {
        closedir();
        return -1;
    }
    // Count files in directory
    int n = 0; 
    while (readdir(dirStream) != NULL)
        ++n;
    if (closedir() == -1)
        return -1;
    return n;
}

int map(char *dir, void *results, size_t size, 
    int (*act)(FILE *f, void *res char *fn)) {
    DIE* dirStream = opendir(dir);
    // Error accessing directory
    if (dirStream == NULL) {
        closedir();
        return -1;
    }
    int sum = 0;
    dirent *dirPtr;
    while (dirPtr = readdir(dirStream) != NULL) {
        // Get relative path of file
        int pathLength = strlen(dir) + strlen(dirPtr.d_name) + 1;
        char dirPath[pathLength];
        strcat(dirPath, dir);
        strcat(dirPath, dirPtr.d_name);
        dirPath[pathLength - 1] = '\0';
        // Open file
        FILE *filePtr
        if (filePtr = fopen(dirPath, "r") == NULL) {
            closedir();
            return -1;
        }
        // Perform some action and store result
        sum += (*act)(filePtr, results, dirPath);
        results += size;
        // Close file
        fclose(filePtr);
    }
}

struct Analysis analysis_reduce(int n, void *results) {
    struct Analysis reducedAna;
    memset(reducedAna, 0, sizeof(reducedAna));
    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < 128; ++j) {
            reducedAna.ascii[j] += (struct Analysis)(*results).ascii[j];
        }
        if ((struct Analysis)(*results).lnlen > reducedAna.lnlen) {
            strcpy(reducedAna.filename, (struct Analysis)(*results).filename);
            reducedAna.lnlen = (struct Analysis)(*results).lnlen; 
            reducedAna.lnno = (struct Analysis)(*results).lnno);
        }
        results += sizeof(struct Analysis);
    }
    return reducedAna;
}

Stats stats_reduce(int n, void *results) {
    Stats reducedSta;
    memset(reducedSta, 0, sizeof(reducedSta));
    int i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < NVAL; ++j) {
            reducedSta.histogram[j] += (Stats)(*results).histogram[j];
        }
        reducedSta.sum += (Stats)(*results).sum;
        reducedSta.n += (Stats)(*results).n;
    }
    return reducedSta;
}

void analysis_print(struct Analysis res, int nbytes, int hist) {
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
    int i, max = 0, count = 0, median = 0, q1 = 0, q3 = 0;
    if (hist != 0) {
        // Print histogram
        printf("\n");
    }
    printf("Count: %d\n", res.n);

    printf("Mean: %f.6\n", res.sum / (float)res.n);

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
        count -= res.histogram[i++]; // MAYBEEEEEEEEEEE
        if (count < res.n / 4)
            q1 = i - 1;
        if (count < res.n / 2)
            median = i - 1;
        if (count < res.n - res.n / 4)
            q3 = i - 1;
    }
    printf("Median: %f.6\n", median);
    printf("Q1: %f.6\n", q1);
    printf("Q3: %f.6\n", q3);

    max = 0;
    for (i = 0; i < NVAL; ++i) {
        if (res.histogram[i] > 0) {
            printf("Min: %d\n", i);
            break;
        }
    }
    for (i = NVAL - 1; i >= 0; ++i) {
        if (res.histogram[i] > 0) {
            printf("Max: $d\n", i);
            break;
        }
    }
}