#include "sfish.h"

#define MAX_EXECS 10
#define PROMPT_SIZE 256
#define HOSTNAME_SIZE 112
#define PWD_SIZE 160
#define CLOSE_TAG "\x1b[0m"

// Environment variables
struct job *jobs_head;
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
    int i = 0, j; // s_print(STDOUT_FILENO, "yo %s %s %s\n", 3, s1, s2, s3);

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

    if (argc == 1 || strncmp(path, "~", 1) == 0) {
        strcpy(new_path_ptr ,getenv("HOME"));
        if (argc != 1 && strcmp(path, "~") != 0) {
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

void* get_builtin(char *cmd, bool *mproc) {
    bool *mp;
    if (mproc != NULL)
        mp = mproc;
    if (strcmp(cmd, "help") == 0) {
        *mp = false; 
        return &sf_help;
    }
    if (strcmp(cmd, "exit") == 0) {
        *mp = true;
        return &sf_exit;
    }
    if (strcmp(cmd, "cd") == 0) {
        *mp = true;
        return &sf_cd;
    }
    if (strcmp(cmd, "pwd") == 0) {
        *mp = false; 
        return &sf_pwd;
    }
    if (strcmp(cmd, "prt") == 0) {
        *mp= false; 
        return &sf_prt;
    }
    if (strcmp(cmd, "chpmt") == 0) {
        *mp = true;
        return &sf_chpmt;
    }
    if (strcmp(cmd, "chclr") == 0) {
        *mp = true;
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

void free_job(struct job *done_job) {
    struct exec *cursor = done_job->exec_head, *temp;
    int i;
    while (cursor != NULL) {
        for (i = 0; i < MAX_ARGS; ++i) {
            if (cursor->argv[i] == NULL)
                break;
            free(cursor->argv[i]);
        }
        temp = cursor->next;
        free(cursor);
        cursor = temp;
    }
    free(done_job->cmd);
    free(done_job);
}

int make_args(char *cmd, int *argc, char **argv) {
    *argc = 0;
    char cmd_cpy[strlen(cmd) + 1], *arg, *argsec;
    strcpy(cmd_cpy, cmd);
    int delim_ind = 0;
    
    // exe arg0 arg1<file0>file1
    while ((argsec = strsep(&cmd, "<>")) != NULL) {
        if (cmd != NULL)
            delim_ind = cmd - argsec - 1;
        else
            delim_ind = 0;
        while ((arg = strsep(&argsec, " ")) != NULL) {
            if (strlen(arg) != 0) {
                argv[*argc] = calloc(strlen(arg) + 1, sizeof(char));
                strcpy(argv[(*argc)++], arg);
            }   
        }
        if (delim_ind > 0) {
            if (cmd_cpy[delim_ind] == '>' || cmd_cpy[delim_ind] == '<') {
                argv[*argc] = calloc(2, sizeof(char));
                argv[(*argc)++][0] = cmd_cpy[delim_ind];
            } 
        }
    }  
    
    
    // Set rest of argv NULL
    int i;
    for (i = *argc; i < MAX_ARGS; ++i) {
        argv[i] = NULL;
    }
    
    // Check for background flag
    if (*argc != 0 && strstr(argv[*argc - 1], "&") != NULL) {
        return 0;
    }
    return 1;
}

int make_job(char *input, struct job **new_job) {
    // Create new_job
    (*new_job) = calloc(1, sizeof(struct job));
    (*new_job)->cmd = input;
    (*new_job)->fg = true;
    
    // Copy input to sep
    char cmd[strlen(input) + 1], *cmdp = cmd;
    strcpy(cmd, input);
    
    if (strlen(input) == 0) {
        free_job(*new_job);
        return 0;
    }

    struct exec *cursor = calloc(1, sizeof(struct exec));

    // Separate by pipe
    char *exec_str;
    while ((exec_str = strsep(&cmdp, "|")) != NULL) {
        if (strlen(exec_str) == 0) {
            s_print(STDERR_FILENO, "Invalid command\n", 0);
            free_job(*new_job);
            return 0;
        }
        // Make new node 
        if ((*new_job)->exec_head == NULL) {
            (*new_job)->exec_head = cursor;
        } else {
            cursor->next = calloc(1, sizeof(struct exec));
            cursor = cursor->next;
        }
        cursor->srcfd = cursor->desfd = -1;
        ++(*new_job)->nexec;
        
        // Fill new args
        if (make_args(exec_str, &cursor->argc, cursor->argv) == false) {
            (*new_job)->fg = false;
        }
        
        // Check for redirection, consume from args
        bool non_args = false;
        char fpb[PWD_SIZE], *fp = fpb;
        memset(fp, 0, PWD_SIZE);
        char cwdb[PWD_SIZE], *cwdp = cwdb;
        getcwd(cwdp, PWD_SIZE);
        for (int i = 1; i < cursor->argc - 1; ++i) {
            var_cat(fp, 3, cwdp, "/", cursor->argv[i + 1]);
            if (strcmp(cursor->argv[i], "<") == 0) {
                if ((cursor->srcfd = open(fp, O_RDONLY)) == -1) {
                    s_print(STDERR_FILENO, "Error opening file '%s'\n", 1,
                    cursor->argv[i + 1]);
                    free_job(*new_job);
                    return 0;
                }
                non_args = true;
            } else if (strcmp(cursor->argv[i], ">") == 0) {
                cursor->desfd = 
                open(fp, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                non_args = true;
            } else if (strcmp(cursor->argv[i], "2>") == 0) {
                cursor->errfd = 
                open(fp, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                non_args = true;
            } else if (strcmp(cursor->argv[i], ">>") == 0) {
                cursor->desfd = 
                open(fp, O_RDWR | O_APPEND | O_CREAT , S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                non_args = true;
            }
            if (non_args) {
                free(cursor->argv[i]);
                cursor->argv[i] = NULL;
            }
            memset(fp, 0, PWD_SIZE);
        }
    }
    return (*new_job)->nexec;
}

void setup_files(struct exec *exec, int *pipes, int npipes, int execn) {
    // Redirect
    if (exec->srcfd != -1) {
        dup2(exec->srcfd, STDIN_FILENO);
        close(exec->srcfd);
    }
    if (exec->desfd != -1) {
        dup2(exec->desfd, STDOUT_FILENO);
        close(exec->desfd);
    }
    if (exec->errfd != -1) {
        dup2(exec->errfd, STDERR_FILENO);
        close(exec->errfd);
    }
    // Pipe
    int pipeind = execn << 1;
    if (pipeind < npipes - 2) {
        dup2(pipes[pipeind + 1], STDOUT_FILENO);
        close(pipes[pipeind + 1]);
    }
    if (pipeind > 0) {
        dup2(pipes[pipeind - 2], STDIN_FILENO);
        close(pipes[pipeind - 2]);
    }
}

void add_job(struct job *new_job) {
    struct job *cursor = jobs_head, *prev_job;
    // Find lowest available jid
    if (cursor == NULL || cursor->jid > 1) {
        new_job->jid = 1;
        new_job->next = jobs_head;
        jobs_head = new_job;
    } else {
        while (cursor != NULL) {
            if (prev_job != NULL && cursor->jid > prev_job->jid + 1) {
                new_job->jid = prev_job->jid + 1;
                prev_job->next = new_job;
                new_job->next = cursor;
                return;
            }
            prev_job = cursor;
            cursor = cursor->next;
        }
        if (new_job->jid == 0) {
            new_job->jid = cursor->jid + 1;
        }
    }
}

void remove_job(struct job *dead_job) {
    struct job *cursor = jobs_head, *prev;
    while (cursor != dead_job) {
        prev = cursor;
        cursor = cursor->next; 
    }
    if (prev != NULL) {
        prev->next = cursor->next;
    } else {
        jobs_head = cursor->next;
    }
    free_job(dead_job);
}   

void start_job(struct job *new_job) {
    struct exec *cursor = new_job->exec_head;

    // Make pipes
    int npipes = (new_job->nexec - 1) << 1, *pipes = calloc(npipes, sizeof(int));
    for (int i = 0; i < npipes << 2; ++i) {
        if (pipe(pipes + (i * 2)) > 0) {
            s_print(STDERR_FILENO, "Error creating pipes\n", 0);             }
    }

    // Fork for all execs
    int execn = 0;
    int (*func)(int, char**);
    while (cursor != NULL) {
        // Exec
        if ((cursor->pid = fork()) == 0) {
            setup_files(cursor, pipes, npipes, execn);
            // Builtin
            if ((func = get_builtin(cursor->argv[0], NULL)) != NULL) {
                (*func)(cursor->argc, cursor->argv);
            }
            // Exec
            else {
            //    if (verify_exec(cursor)) {
                    if(execvp(cursor->argv[0], cursor->argv)) {
                        s_print(STDERR_FILENO, "%s: command not found\n", 1, 
                        cursor->argv[0]);
                        exit(EXIT_SUCCESS);        
                    }
           //     } 
                // Invalid exec
                else {
                    s_print(STDERR_FILENO, "%s: command not found\n", 1, 
                    cursor->argv[0]);
                    exit(EXIT_SUCCESS);
                }
            } 
        } 
        // // Job parent
        // else {
            
        //     if (waitpid(pid, &status, 0) < 0) {
        //         s_print(STDERR_FILENO, "waitpid error\n", 0);
        //         errno = prev_errno;
        //     }
        // }
        ++execn;
    }
    
    // Close all pipes
    for (int i = 0; i < npipes; ++i) {
        close(pipes[i]);
    }

    // Wait for all execs to die
    int prev_errno = errno, status;
    cursor = new_job->exec_head;
    for (int i = 0; i < new_job->nexec; ++i) {
        if (waitpid(cursor->pid, &status, 0) == -1) {
            s_print(STDERR_FILENO, "waitpid error\n", 0);
            errno = prev_errno;
        }
    }
}

void eval_cmd(char *input) {
    struct job *new_job;

    // Create job and and check for no command
    if (!make_job(input, &new_job)) {
        return;
    }

    // Check if job is main process builtin
    bool mproc;
    int (*func)(int, char**) = get_builtin(new_job->exec_head->argv[0], &mproc);
    if (func != NULL && mproc) {
        (*func)(new_job->exec_head->argc, new_job->exec_head->argv);
        free_job(new_job);
        return;
    }

    // Add job to job list
    add_job(new_job);

    // Fork for job and start
    if ((new_job->pid = fork()) == 0) {
        start_job(new_job);
        exit(EXIT_SUCCESS);
    } 

    // Foreground: wait for job to end
    if (new_job->fg) {
        int status, prev_errno = errno;
        if (waitpid(new_job->pid, &status, 0) == -1) {
            s_print(STDERR_FILENO, "waitpid error\n", 0);
            errno = prev_errno;
        }
        
    } 
}

void sigint_handler(int sig) {
    int prev_errno = errno;
    sigset_t int_mask, prev_mask;

    // Block sigint
    sigemptyset(&int_mask);
    sigaddset(&int_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &int_mask, &prev_mask);

    // Tell foreground job to interrupt


    // Unblock sigint
    sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    
    errno = prev_errno;
} 

void sigchld_handler(int sig) {
    int prev_errno = errno, status;
    sigset_t chld_mask, prev_mask;
    pid_t pid;

    // // Check if responsible child is background
    // struct job *job_cursor = job_head;
    // while (job_cursor != NULL) {
    //     if (job_cursor->fg)
    //         return;
    //     job_cursor = job_cursor->next;
    // }
    
    // Block sigchld
    sigemptyset(&chld_mask);
    sigaddset(&chld_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &chld_mask, &prev_mask);

    // Reap all dead children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Remove pid from job list
        // remove_job(pid);
        printf("HEIM %d\n", pid);
    }

    // Unblock sigchld
    sigprocmask(SIG_SETMASK, &prev_mask, NULL);

    errno = prev_errno;
}

int main(int argc, char** argv) {
    //DO NOT MODIFY THIS. If you do you will get a ZERO.
    rl_catch_signals = 0;
    //This is disable readline's default signal handlers, since you are going
    //to install your own.
    signal(SIGCHLD, sigchld_handler);

    pwd = calloc(PWD_SIZE, sizeof(char));
    machine = calloc(HOSTNAME_SIZE, sizeof(char));
    char *prompt = calloc(PROMPT_SIZE, sizeof(char));
    make_prompt(prompt);

    // Set sig handlers
    // set_handlers();

    char *test2 = calloc(100, 1);
    strcpy(test2, "");
    eval_cmd(test2);

    char *test1 = calloc(100, 1);
    strcpy(test1, "ls");
    eval_cmd(test1);

    char *test3 = calloc(100, 1);
    strcpy(test3, "");
    eval_cmd(test3);

    char *test4 = calloc(100, 1);
    strcpy(test4, "");
    eval_cmd(test4);

    // char *test = calloc(20, 1); grep - < hello | cowsay | grep ^ | cowsay > madness
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
