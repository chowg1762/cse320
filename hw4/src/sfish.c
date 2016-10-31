#include "sfish.h"

#define MAX_ARGS 15
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
bool user_tag = true;
bool mach_tag = true;

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
    strcpy(new_path_ptr, path);

    if (argc == 1 || strncmp(path, "~", 1) == 0) {
        strcpy(new_path_ptr ,getenv("HOME"));
        if (strcmp(path, "~") != 0) {
            size_t homelen = strlen(new_path_ptr);
            strncpy(new_path_ptr + homelen, path, strlen(path) - 2);
        }
    } else if (strcmp(path, "-") == 0) {
        if (strlen(last_dir) != 0) {
            new_path_ptr = last_dir;
        } else {
            new_path_ptr = pwd;
        }
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

int get_exec(char *exec, char *path_buf) {
    struct stat stats;
    // Direct location
    if (strstr(exec, "/") != NULL) {
        var_cat(path_buf, 3, pwd, "/", exec);
        if (stat(path_buf, &stats) == -1) {
            return 1;
        } else {
            return 0;
        }
    }

    // Unspecified location
    char path_list[PWD_SIZE], *path_ptr = path_list, *cur_dir;
    strcpy(path_list, getenv("PATH"));
    while ((cur_dir = strsep(&path_ptr, ":")) != NULL) {
        var_cat(path_buf, 3, cur_dir, "/", exec);  
        if (stat(path_buf, &stats) != -1) {
            return 1;
        }
        memset(path_buf, 0, PWD_SIZE);
    }
    return 0;
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
    char *arg;
    while ((arg = strsep(&cmd, " ")) != NULL) {
        if (strlen(arg) != 0) {
            argv[*argc] = calloc(strlen(arg) + 1, sizeof(char));
            strcpy(argv[*argc], arg);
            ++(*argc);
        }
    }
    // Set rest of argv NULL
    int i;
    for (i = *argc; i < MAX_ARGS; ++i) {
        argv[i] = NULL;
    }
    // Check for background flag
    if (strcmp(argv[*argc - 1], "&") == 0) {
        return 0;
    }
    return 1;
}

void free_args(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; ++i) {
        free(argv[i]);
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

void eval_cmd(char *cmd) {
    int argc;
    char *argv[MAX_ARGS];
    bool fg;
    pid_t pid;

    // Parse cmd and alloc for argv
    fg = make_args(cmd, &argc, argv);  

    // No command
    if (argc == 0) {
        return;
    }

    // Job - TODO

    // Builtin
    int (*func)(int, char**);
    char path[PWD_SIZE];
    if ((func = get_builtin(argv[0])) != NULL) {
        (*func)(argc, argv); // Fork for output redir
    }

    // Exec
    else if (get_exec(argv[0], path)) {
        // Child
        if ((pid = fork()) == 0) {
            execv(path, argv);
            //execv("/bin/ls", argv);
            printf("errno: %d\n", errno);
        } 
        // Parent - foreground
        else if (fg) {
            int status;
            if (waitpid(pid, &status, 0) < 0)
                s_print(STDERR_FILENO, "waitpid error\n", 0);
        } 
        // Parent - background
        else {
            s_print(STDOUT_FILENO, "%d: %s\n", 2, pid, cmd);
        }
        free(cmd);
    } 

    // Invalid
    else {
        s_print(STDERR_FILENO, "%s: command not found\n", 1, argv[0]);
    }

    free_args(argc, argv);
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

    // char *test = calloc(7, 1);
    // test[0] = 'c';
    // test[1] = 'd';
    // test[2] = ' ';
    // test[3] = '~';
    // test[4] = '/';
    // test[5] = 'p';
    // test[6] = '\0';
    // eval_cmd(test);

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
