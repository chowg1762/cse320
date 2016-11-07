#ifndef SFISH_H
#define SFISH_H

#include <errno.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 15
// #define SIGHUP       	1
// #define SIGINT       	2
// #define SIGQUIT      	3
// #define SIGILL       	4
// #define SIGTRAP      	5
// #define SIGABRT      	6
// #define SIGIOT       	6
// #define SIGBUS       	7
// #define SIGFPE       	8
// #define SIGKILL      	9
// #define SIGUSR1     	10
// #define SIGSEGV     	11
// #define SIGUSR2     	12
// #define SIGPIPE     	13
// #define SIGALRM     	14
// #define SIGTERM     	15
// #define SIGSTKFLT   	16
// #define SIGCHLD     	17
// #define SIGCONT     	18
// #define SIGSTOP     	19
// #define SIGTSTP     	20
// #define SIGTTIN     	21
// #define SIGTTOU     	22
// #define SIGURG      	23
// #define SIGXCPU     	24
// #define SIGXFSZ     	25
// #define SIGVTALRM   	26
// #define SIGPROF     	27
// #define SIGWINCH    	28
// #define SIGIO       	29
// #define SIGPWR      	30  
// #define SIGSYS      	31

enum colors {BLACK = 0, B_BLACK, RED, B_RED, GREEN, B_GREEN, 
YELLOW, B_YELLOW, BLUE, B_BLUE, MAGENTA, B_MAGENTA, CYAN, B_CYAN, WHITE, B_WHITE};

char *open_tags[16] = {
    "\e[0;30m", // Black
    "\e[1;30m", 
    "\e[0;31m", // Red
    "\e[1;31m",
    "\e[0;32m", // Green
    "\e[1;32m",
    "\e[0;33m", // Yellow
    "\e[1;33m",
    "\e[0;34m", // Blue
    "\e[1;34m",
    "\e[0;35m", // Magenta
    "\e[1;35m",
    "\e[0;36m", // Cyan
    "\e[1;36m",
    "\e[0;37m", // White
    "\e[1;37m"
};

struct exec {
    pid_t pid;
    int argc;
    char *argv[MAX_ARGS];
    int srcfd;
    int desfd;
    int errfd;
    struct exec *next;
};

struct job {
    int jid;
    pid_t pid;
    char *cmd;
    char *status;
    bool fg;
    int nexec;
    struct exec *exec_head;
    struct job *next;
};

enum status {RUNNING = 0, STOPPED};
char *exec_status[2] = {"Running", "Stopped"};

#endif

// int parse_args(char *input, struct exec **args_head) {
//     // Copy input to sep
//     char cmd[strlen(input) + 1], *cmdp = cmd;
//     strcpy(cmd, input);
    
//     if (strlen(input) == 0) {
//         return 0;
//     }

//     int nexec = 0;
//     struct args_node *args_cursor = calloc(1, sizeof(struct args_node));

//     // Separate by pipe
//     char *exec;
//     while ((exec = strsep(&cmdp, "|")) != NULL) {
//         if (strlen(exec) == 0) {
//             s_print(STDERR_FILENO, "Invalid command\n", 0);
//             free_args(*args_head);
//             return 0;
//         }
//         // Make new node 
//         if (*args_head == NULL) {
//             *args_head = args_cursor;
//         } else {
//             args_cursor->next = calloc(1, sizeof(struct args_node));
//             args_cursor = args_cursor->next;
//         }
//         args_cursor->srcfd = args_cursor->desfd = -1;
//         ++nexec;
//         // Fill new args
//         args_cursor->fg = make_args(exec, &args_cursor->argc, args_cursor->argv);
        
//         // Check for redirection, consume from args
//         int i;
//         bool non_args = false;
//         char fpb[PWD_SIZE], *fp = fpb;
//         memset(fp, 0, PWD_SIZE);
//         char cwdb[PWD_SIZE], *cwdp = cwdb;
//         getcwd(cwdp, PWD_SIZE);
//         for (i = 1; i < args_cursor->argc - 1; ++i) {
//             var_cat(fp, 3, cwdp, "/", args_cursor->argv[i + 1]);
//             if (strcmp(args_cursor->argv[i], "<") == 0) {
//                 if ((args_cursor->srcfd = open(fp, O_RDONLY)) == -1) {
//                     s_print(STDERR_FILENO, "Error opening file '%s'\n", 1,
//                     args_cursor->argv[i + 1]);
//                     free_args(*args_head);
//                     return 0;
//                 }
//                 non_args = true;
//             } else if (strcmp(args_cursor->argv[i], ">") == 0) {
//                 args_cursor->desfd = 
//                 open(fp, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
//                 non_args = true;
//             } else if (strcmp(args_cursor->argv[i], "2>") == 0) {
//                 args_cursor->errfd = 
//                 open(fp, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
//                 non_args = true;
//             } else if (strcmp(args_cursor->argv[i], ">>") == 0) {
//                 args_cursor->desfd = 
//                 open(fp, O_RDWR | O_APPEND | O_CREAT , S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
//                 non_args = true;
//             }
//             if (non_args) {
//                 free(args_cursor->argv[i]);
//                 args_cursor->argv[i] = NULL;
//             }
//             memset(fp, 0, PWD_SIZE);
//         }
//     }
//     return nexec;
// }

//     // Create pipes for job
//     int pipes[(nexec - 1) * 2], i;
//     if (nexec > 1) {
//         for (i = 0; i < nexec - 1; ++i) {
//             if (pipe(pipes + (i * 2)) > 0) {
//                 s_print(STDERR_FILENO, "Error creating pipes\n", 0);
//             }
//         }
//     } else {
//         pipes[0] = -1;
//     }

//     // Job - TODO


//     // Builtin
//     int (*func)(int, char**);
//     if ((func = get_builtin(args_cursor->argv[0])) != NULL) {
//         // Redirection
//         if (args_cursor->desfd != -1) {
//             // Child
//             if ((pid = fork()) == 0) {
//                 if (args_cursor->desfd != -1) {
//                     dup2(args_cursor->desfd, STDOUT_FILENO);
//                     close(args_cursor->desfd);
//                 }
//               (*func)(args_cursor->argc, args_cursor->argv);
//               exit(0); 
//             } 
//             // Parent
//             else {
//                 // Close
//                 if (args_cursor->srcfd != -1) {
//                     close(args_cursor->srcfd);
//                 }
//                 if (args_cursor->desfd != -1) {
//                     close(args_cursor->desfd);
//                 }
//                 int status;
//                 if (waitpid(pid, &status, 0) < 0) {
//                     s_print(STDERR_FILENO, "waitpid error\n", 0);
//                 }
//             } 
//         } 
//         // No Redirection
//         else {
//             (*func)(args_cursor->argc, args_cursor->argv);
//         }
//     }

//     // Executable
//     else {
//         i = 0;
//         while (args_cursor != NULL) {
//             // Child
//             if ((pid = fork()) == 0) {
//                 // Pipes
//                 if (pipes[0] != -1) {
//                     if (i > 0 && i < (nexec - 1) << 1) {
//                         dup2(pipes[i - 2], STDIN_FILENO);
//                         dup2(pipes[i + 1], STDOUT_FILENO);
//                         // close(pipes[i - 1]);
//                         // close(pipes[i + 2]); // a:1, b:03, c:2
//                     } else if (i == 0) {
//                         dup2(pipes[1], STDOUT_FILENO);
//                         // close(pipes[1]); 
//                     } else {
//                         dup2(pipes[((nexec - 1) * 2) - 2], STDIN_FILENO);
//                         // close(pipes[((nexec - 1) * 2) - 2]);
//                     }
//                     for (int j = 0; j < (nexec - 1) * 2; ++j) {
//                         close(pipes[j]);
//                     }
//                 }
//                 // Non-Pipes
//                 if (args_cursor->srcfd != -1) {
//                     dup2(args_cursor->srcfd, STDIN_FILENO);
//                     close(args_cursor->srcfd);
//                 }
//                 if (args_cursor->desfd != -1) {
//                     dup2(args_cursor->desfd, STDOUT_FILENO);
//                     close(args_cursor->desfd);
//                 }
//                 // Execute
//                 if (execvp(args_cursor->argv[0], args_cursor->argv)) {
//                     // Invalid
//                     s_print(STDERR_FILENO, "%s: command not found\n", 1, 
//                         args_cursor->argv[0]);
//                     exit(EXIT_SUCCESS);
//                 }        
//             } 
//             // Parent
//             else {
//                 // Close pipe if needed
//                 if (pipes[0] != -1) {
//                     if (i > 0 && i < (nexec - 1) << 1) {
//                         close(pipes[i - 2]);
//                         close(pipes[i + 1]);
//                     } else if (i == 0) {
//                         close(pipes[1]);
//                     } else {
//                         close(pipes[((nexec - 1) * 2) - 2]);
//                     }
//                 }
            
//                 // Foreground
//                 if (args_cursor->fg) {
//                     int status, prev_errno = errno;
//                     if (waitpid(pid, &status, 0) < 0) {
//                         s_print(STDERR_FILENO, "waitpid error\n", 0);
//                         errno = prev_errno;
//                     }
//                 } 
//                 // Background
//                 else {
//                     s_print(STDOUT_FILENO, "%d: %s\n", 2, pid, input);
//                 }
//             }
//             args_cursor = args_cursor->next;
//             i += 2;
//         }
//         // Close all redirection files
//         args_cursor = args_head;
//         while (args_cursor != NULL) {
//             if (args_cursor->srcfd != -1) {
//                 close(args_cursor->srcfd);
//             }
//             if (args_cursor->desfd != -1) {
//                 close(args_cursor->srcfd);
//             }
//             args_cursor = args_cursor->next;
//         }
//     }

//     free_args(args_head);
// }