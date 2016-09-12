#include "map_reduce.h"
#include <string.h>
#include <dirent.h>

//Space to store the results for analysis map
struct Analysis analysis_space[NFILES];
//Space to store the results for stats map
Stats stats_space[NFILES];

//Sample Map function action: Print file contents to stdout and returns the number bytes in the file.
int cat(FILE *f, void *res, char *filename) {
    char c;
    int n = 0;
    printf("%s\n", filename);
    while((c = fgetc(f)) != EOF) {
        printf("%c", c);
        n++;
    }
    printf("\n");
    return n;
}

int main(int argc, char **argv) {
    int runChoice = validateargs(argc, argv);
    switch (runChoice) {
        case -1:
            return EXIT_FAILURE;
        case 0:
            return EXIT_SUCCESS;
    }
    int numFiles = nfiles(argv[argc - 1]);
    void *results, *act;
    size_t size;
    switch (runChoice) {
        case 1: // Analysis
            results = analysis_space;
            act = analysis_reduce;
            size = sizeof(Struct Analysis)    
    }
    map(argv[argc - 1], placeHolder, placeHolderSize, cat);
   // reduce();
   // print();
    return EXIT_SUCCESS;
}
