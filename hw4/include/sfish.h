#ifndef SFISH_H
#define SFISH_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

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

#endif