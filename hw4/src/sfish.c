#include "sfish.h"

#define MAX_EXECS 10
#define PROMPT_SIZE 256
#define HOSTNAME_SIZE 112
#define PWD_SIZE 160
#define CLOSE_TAG "\x1b[0m"

// Environment variables
char last_dir[256];
int last_return;

// Prompt settings
char *user;
char *user_color = "\e[0;37m"; // Default white non-bold
char *machine;
char *machine_color = "\e[0;37m"; // Default white non-bold
char *pwd;
bool user_tag = false;
bool mach_tag = false;

char *var_cat(char *buf, int nvar, ...) {
    va_list vars;
    va_start(vars, nvar);
    for (; nvar > 0; --nvar) {
        strcat(buf, va_arg(vars, char*));
    }
    va_end(vars);
    return buf;
}

void s_print(int fd, const char *format, int nvar, ...) {
    // Count vars in format
    int i = 0, j;

    // Initialize va_list
    va_list vars;
    va_start(vars, nvar);

    // Make variable length string on stack for print
    char str[1000];
    memset(str, 0, 1000);

    i = j = 0;
    while (format[i] != '\0') {
        if (format[i] == '%') {
            // Concat substring up to '%'
            strncat(str, (format + i - j), j);
            // Concat var
            strcat(str, va_arg(vars, const char*));
            ++i;
            j = 0;
        } else {
            ++j;
        }
        ++i;
    }
    strncat(str, (format + i - j), j);

    // Close va_list
    va_end(vars);

    // Fork
    write(fd, str, strlen(str));
}

char *strtrim(char *input) {
    char *trimmed;
    while ((trimmed = strsep(input, " ")) != NULL) {
        if (strlen(trimmed) != 0) {
            break;
        }
    }
    return trimmed;
}

void update_pwd() {
    getcwd(pwd, PWD_SIZE);
    
    // Handle home shortening
    char *home = getenv("HOME");
    int home_len = strlen(home);
    if (strncmp(pwd, home, home_len) == 0) {
        char dir_temp[PWD_SIZE];
        strcpy(dir_temp, pwd + home_len);
        pwd[0] = '~';
        strcpy(pwd + 1, dir_temp);   
    }
}

int sf_help(int argc, char **argv) {
    // Print help menu
    return 0;
}

void sf_exit(int argc, char **argv) {
    exit(EXIT_SUCCESS);
}

int sf_cd(int argc, char **argv) {
    char *path = argv[1];

    // Save old pwd
    char prev_pwd[PWD_SIZE];
    strcpy(prev_pwd, pwd);

    // Make new changable path
    char new_path[PWD_SIZE], *new_path_ptr = new_path;
    memset(new_path, 0, PWD_SIZE);

    if (argc == 1 || strncmp(path, "~", 1) == 0) {
        strcpy(new_path_ptr ,getenv("HOME"));
        if (strcmp(path, "~") != 0) {
            size_t homelen = strlen(new_path_ptr);
            strncpy(new_path_ptr + homelen, path + 1, strlen(path) - 1);
        }
    } else if (strcmp(path, "-") == 0) {
        if (strlen(last_dir) != 0) {
            new_path_ptr = last_dir;
        } else {
            new_path_ptr = pwd;
        }
    } else {
        strcpy(new_path_ptr, path);
    }

    if (chdir(new_path) == -1) {
        s_print(STDERR_FILENO, "sfish: cd: %s: No such directory\n", 1, argv[1]);
        return 1;
    }
    update_pwd();
    strcpy(last_dir, prev_pwd);

    return 0;
}

int sf_pwd(int argc, char **argv) {
    s_print(STDOUT_FILENO, "%s\n", 1, pwd);
    return 0;
}

int sf_prt(int argc, char **argv) {
    s_print(STDOUT_FILENO, "%d\n", 1, last_return);
    return 0;
}

int sf_chpmt(int argc, char **argv) {
    int val;
    
    // Check field and value
    if (argc != 3 || ((val = atoi(argv[2])) != 0 && val != 1)) {
        s_print(STDERR_FILENO, "chpmt: Invalid input\n", 0);
        return 1;
    }

    // Set new value
    if (strcmp(argv[1], "user") == 0) {
        user_tag = val;
    } else if (strcmp(argv[1], "machine") == 0) {
        mach_tag = val;
    } else {
        s_print(STDERR_FILENO, "chpmt: Invalid input\n", 0);
        return 1;
    }
    return 0;
}

int sf_chclr(int argc, char **argv) {
    char **setting, *color;
    int bold;
    
    // Check for valid input
    if (argc != 4 || (strcmp(argv[3], "0") != 0 && 
    strcmp(argv[3], "1") != 0)) {
        s_print(STDERR_FILENO, "chclr: Invalid input\n", 0);
        return 1;
    }

    bold = atoi(argv[3]);

    // Check setting
    if (strcmp(argv[1], "user") == 0) {
        setting = &user_color;
    } else if (strcmp(argv[1], "machine") == 0) {
        setting = &machine_color;
    } else {
        s_print(STDERR_FILENO, "chclr: Invalid input\n", 0);
        return 1;
    }

    // Check color
    if (strcmp(argv[2], "blue") == 0) {
        color = open_tags[bold + BLUE];
    } else if (strcmp(argv[2], "black") == 0) {
        color = open_tags[bold + BLACK];
    } else if (strcmp(argv[2], "cyan") == 0) {
        color = open_tags[bold + CYAN];
    } else if (strcmp(argv[2], "green") == 0) {
        color = open_tags[bold + GREEN];
    } else if (strcmp(argv[2], "magenta") == 0) {
        color = open_tags[bold + MAGENTA];
    } else if (strcmp(argv[2], "red") == 0) {
        color = open_tags[bold + RED];
    } else if (strcmp(argv[2], "white") == 0) {
        color = open_tags[bold + WHITE];
    } else if (strcmp(argv[2], "yellow") == 0) {
        color = open_tags[bold + YELLOW];
    } else {
        s_print(STDERR_FILENO, "chclr: Invalid input\n", 0);
        return 1;
    }

    // Set color
    *setting = color;

    return 0;
}

void* get_builtin(char *cmd) {
    // Check for piping

    if (strcmp(cmd, "help") == 0) {
        return &sf_help;
    }
    if (strcmp(cmd, "exit") == 0) {
        return &sf_exit;
    }
    if (strcmp(cmd, "cd") == 0) {
        return &sf_cd;
    }
    if (strcmp(cmd, "pwd") == 0) {
        return &sf_pwd;
    }
    if (strcmp(cmd, "prt") == 0) {
        return &sf_prt;
    }
    if (strcmp(cmd, "chpmt") == 0) {
        return &sf_chpmt;
    }
    if (strcmp(cmd, "chclr") == 0) {
        return &sf_chclr;
    }
    return NULL;
}

void make_prompt(char* prompt) {
    // Fix sfish prompt
    memset(prompt, 0, PROMPT_SIZE);
    strcat(prompt, "sfish");

    if (user == NULL) {
        user = getenv("USER");
    }
    if (strlen(machine) == 0) {
        gethostname(machine, HOSTNAME_SIZE);
    }
    if (strlen(pwd) == 0) {
        update_pwd();
    }

    // Add user and/or machine
    if (user_tag) {
        var_cat(prompt, 4, "-", user_color, user, CLOSE_TAG);
        if (mach_tag) {
            var_cat(prompt, 5, "@", machine_color, machine, CLOSE_TAG, ":");
        } else {
            strcat(prompt, ":");
        }
    } else if (mach_tag) {
        var_cat(prompt, 5, "-", machine_color, machine, CLOSE_TAG, ":");
    } else {
        strcat(prompt, ":");
    }

    // Add pwd
    var_cat(prompt, 3, "[", pwd, "]>");
}

int make_args(char *cmd, int *argc, char **argv) {
    *argc = 0;
    char *arg, *argsec, *tok, **tokp = &tok;
    
    // exe arg0 arg1<file0>file1
    if ((argsec = strtok_r(cmd, "<>", tokp)) != NULL) {
        do {
            while ((arg = strsep(&argsec, " ")) != NULL) {
                if (strlen(arg) != 0) {
                    argv[*argc] = calloc(strlen(arg) + 1, sizeof(char));
                    strcpy(argv[*argc], arg);
                    ++(*argc);
                }   
            }
        } while ((argsec = strtok_r(NULL, "<>", tokp)) != NULL); 
    }
    
    // Set rest of argv NULL
    int i;
    for (i = *argc; i < MAX_ARGS; ++i) {
        argv[i] = NULL;
    }
    
    // Check for background flag
    if (*argc != 0 && strcmp(argv[*argc - 1], "&") == 0) {
        return 0;
    }
    return 1;
}

int parse_args(char *input, struct args *args_head) {
    // Copy input to sep
    char cmd[strlen(input) + 1];
    strcpy(cmd, input);
    
    if (strlen(input) == 0) {
        return 0;
    }

    int nexec = 0;
    struct args *args_cursor = args_head, *args_prev;
    args_cursor = calloc(1, sizeof(struct args));

    // Separate by pipe
    char *exec, *seg, *tok, **tokp = &tok;
    while ((exec = strsep(cmd, "|")) != NULL) {
        // Add node to list
        args_cursor = calloc(1, sizeof(struct args));
        args_cursor->prev = args_prev;
        if (args_prev != NULL) {
            args_prev->next = args_cursor;
        }
        ++nexec;
        // Fill new args
        make_args(exec, &args_cursor->argc, args_cursor->argv);
        // Check for redirection
        int i;
        for (i = 0; i < args_cursor->argc; ++i) {
            if (strcmp(args_cursor->argv[i], "<") == 0) {
                agrs_cursor->srcfd = open(args_cursor->argv[++i], O_RDONLY);
            } else if (strcmp(args_cursor->argv[i], ">") == 0) {
                agrs_cursor->desfd = open(args_cursor->argv[++i], O_WRONLY);
            } else if (strcmp(args_cursor->argv[i], "2>") == 0) {
                args_cursor->desfd = STDERR_FILENO;
            } else if (strcmp(args_cursor->argv[i], ">>") == 0) {
                args_cursor->desfd = open(args_cursor->argv[i++], O_WRONLY | O_APPEND)
            }
        }
        if ((seg = strtok_r(exec, "<>", tokp)) != NULL) {
            do {
                seg = strtrim(seg);
                if (strncmp(seg - 1, "<", 1) == 0) {
                    
                } else if (strncmp(seg - 1, ">", 1) == 0) {

                }
            } while ((seg = strtok_r(NULL, "<>", tokp)) != NULL);
        }
        args_cursor = args_cursor->next;
    }
    return nexec;
}

void free_args(struct args_node *cursor) {
    struct args_node *temp;
    int i;
    while (cursor != NULL) {
        for (i = 0; i < MAX_ARGS; ++i) {
            if (cursor->argv[i] == NULL)
                break;
            free(cursor->argv[i])
        }
        temp = cursor->next;
        free(cursor);
        cursor = temp;
    }
}

void sigint_handler(int sig) {
    int prev_errno = errno;
    //pid_t pid;

    // Interrupt FG process
    errno = prev_errno;
}

// void sigchild_handler(int sig) {
//     int prev_errno = errno;
//     pid_t pid;
//     // Reap all dead children
//     while ((pid = wait(NULL)) > 0) {

//     }

//     errno = prev_errno;
// }

void eval_cmd(char *input) {
    struct args_node *args_head, *args_cursor = args_head;
    bool fg;
    pid_t pid;
    bool first = true;

    // Parse cmd and alloc for argv
    int srcfd, desfd;
    char *cmd, *tok, **tokp = &tok;
    if ((cmd = strtok_r(input, "|", tokp)) != NULL) {
        do {
            // Make new args_node 
            args_cursor = calloc(1, sizeof(struct args_node));
            fg = make_args(cmd, &args_cursor->argc, args_cursor->argv);

            // Redir from/to file
            if (!first && (strncmp(cmd - 1, "<", 1) == 0 || 
            strncmp(cmd - 1, ">", 1) == 0)) {
                // Trim whitespace
                char *filename;
                while (filename = strsep(cmd, " ") != NULL) {
                    if (strlen(filename) != 0) {
                        break;
                    }
                }
                // No file given
                if (filename == NULL) {
                    s_print(STDERR_FILENO, "Invalid command: %s", 1, input);
                    free_args(args_head);
                    return;
                }
                if (strncmp(cmd - 1, "<", 1) == 0) {
                    args_cursor->srcfd = open(filename, O_RDONLY);
                    args_cursor->desfd = STDOUT_FILENO;
                } else {
                    if (strncmp((cmd - 2), "2", 1) == 0) {
                        args_cursor->srcfd = STDERR_FILENO;
                        args_cursor->desfd = STDOUT_FILENO;
                    } else {
                        args_cursor->srcfd = STDOUT_FILENO;
                        args_cursor->desfd = open(filename, O_WRONLY);
                    }
                } 
            }
            // Read next executable
            else if (!first && strncmp(cmd - 1), "|", 1) == 0) {
                if (cmd = strtok_r(NULL, "|", tokp) != NULL) {
                    struct args_node *args_new = 
                        calloc(1, sizeof(struct args_node));
                    make_args(cmd, &args_new->argc, args_cursor->argv);
                } else {
                    s_print(STDERR_FILENO, "Invalid command: %s", 1, input);
                    free_args(args_head);
                    return;
                }
            }


        } while ((cmd = strtok_r(NULL, "<>|", tokp)) != NULL);
    }

    // No command
    int nexec;
    if ((nexec = parse_args(input)) == 0) {
        return;
    }

    // Job - TODO

    // Builtin
    int (*func)(int, char**);
    if ((func = get_builtin(args_list[0].argv[0])) != NULL) {
        (*func)(argc, argv); // Fork for output redir
    }

    // Executable
    else {
        // Spawn all children
        argc_cursor = args_head;
        while (args_cursor != NULL) {
            // Child
            if ((pid = fork()) == 0) {
                // Set redirections

                if (execvp(args_cursor->argv[0], args_cursor->argv)) {
                    // Invalid
                    s_print(STDERR_FILENO, "%s: command not found\n", 1, 
                        args_cursor->argv[0]);
                    exit(EXIT_SUCCESS);
                }        
            } 
            // Parent - foreground
            else if (fg) {
                int status;
                if (waitpid(pid, &status, 0) < 0) {
                    s_print(STDERR_FILENO, "waitpid error\n", 0);
                }
            } 
            // Parent - background
            else {
                s_print(STDOUT_FILENO, "%d: %s\n", 2, pid, input);
            }
            args_cursor = args_cursor->next;
        }
    }

    free_args(args_head);
}

int main(int argc, char** argv) {
    //DO NOT MODIFY THIS. If you do you will get a ZERO.
    rl_catch_signals = 0;
    //This is disable readline's default signal handlers, since you are going
    //to install your own.
    pwd = calloc(PWD_SIZE, sizeof(char));
    machine = calloc(HOSTNAME_SIZE, sizeof(char));
    char *prompt = calloc(PROMPT_SIZE, sizeof(char));
    make_prompt(prompt);

    // Set sig handlers
    // set_handlers();

    // char *test2 = calloc(20, 1);
    // strcpy(test2, "");
    // eval_cmd(test2);

    // char *test = calloc(20, 1);
    // strcpy(test, "cd ..");
    // eval_cmd(test);

    // char *test3 = calloc(20, 1);
    // strcpy(test3, "ls");
    // eval_cmd(test3);

    // char *test4 = calloc(20, 1);
    // strcpy(test4, "cd ..");
    // eval_cmd(test4);

    char *cmd;
    while((cmd = readline(prompt)) != NULL) {
        eval_cmd(cmd);
        make_prompt(prompt);
        free(cmd);
    }

    //Don't forget to free allocated memory, and close file descriptors.
    free(pwd);
    free(machine);
    free(prompt);
    //WE WILL CHECK VALGRIND!

    return EXIT_SUCCESS;
}
