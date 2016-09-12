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
    int nBits;
    switch (runChoice) {
        case 1: // Analysis
            nBits = map(argv[argc - 1], analysis_space, sizeof(struct Analysis), &analysis);
            if (nBits == -1) {
                return EXIT_FAILURE;
            }
            struct Analysis reducedAna = analysis_reduce(numFiles, analysis_space);
            analysis_print(reducedAna, nBits, 1);
            break;
        case 2: // Stats
            nBits = map(argv[argc - 1], stats_space, sizeof(Stats), &stats);
            if (nBits == -1) {
                return EXIT_FAILURE;
            }
            Stats reducedStats = stats_reduce(numFiles, stats_space);
            stats_print(reducedStats, 1);
            break;
        case 3: // Analysis w/ flag
            nBits = map(argv[argc - 1], analysis_space, sizeof(struct Analysis), &analysis);
            if (nBits == -1) {
                return EXIT_FAILURE;
            }
            struct Analysis reducedAnalysis = analysis_reduce(numFiles, analysis_space);
            analysis_print(reducedAna, nBits, 1);
        case 4: // Stats w/ flag
            nBits = map(argv[argc - 1], stats_space, sizeof(Stats), &stats);
            if (nBits == -1) {
                return EXIT_FAILURE;
            }
            Stats reducedStats = stats_reduce(numFiles, stats_space);
            stats_print(reducedStats, 1);
    }
    return EXIT_SUCCESS;
}
