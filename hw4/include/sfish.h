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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 15
#define SIGHUP       	1
#define SIGINT       	2
#define SIGQUIT      	3
#define SIGILL       	4
#define SIGTRAP      	5
#define SIGABRT      	6
#define SIGIOT       	6
#define SIGBUS       	7
#define SIGFPE       	8
#define SIGKILL      	9
#define SIGUSR1     	10
#define SIGSEGV     	11
#define SIGUSR2     	12
#define SIGPIPE     	13
#define SIGALRM     	14
#define SIGTERM     	15
#define SIGSTKFLT   	16
#define SIGCHLD     	17
#define SIGCONT     	18
#define SIGSTOP     	19
#define SIGTSTP     	20
#define SIGTTIN     	21
#define SIGTTOU     	22
#define SIGURG      	23
#define SIGXCPU     	24
#define SIGXFSZ     	25
#define SIGVTALRM   	26
#define SIGPROF     	27
#define SIGWINCH    	28
#define SIGIO       	29
#define SIGPWR      	30  
#define SIGSYS      	31

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

struct args_node {
    int argc;
    char *argv[MAX_ARGS];
    int srcfd;
    int desfd;
    int errfd;
    bool fg;
    struct args_node *next;
};

struct job {
    int jid;
    int pid;
    char *cmd;
    char *status;
    bool fg;
};

char *exec_status[2] = {"Running", "Stopped"};

#endif