#include "shell.h"

void parse_input(char *input, char **args) {
    char *token = strtok(input, " \t\n");
    int ix = 0;
    while (token != NULL) {
        args[ix++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[ix] = NULL;
}

void expand_var(char *input) {
    char expanded[MAX_INPUT] = {0};
    char *input_pointer = input, *expanded_pointer = expanded;

    while (*input_pointer) {
        if (*input_pointer == '$') {
            char var_name[MAX_INPUT] = {0};
            input_pointer++;
            char *var_start = input_pointer;
            while ((*input_pointer >= 'A' && *input_pointer <= 'Z') ||
                   (*input_pointer >= 'a' && *input_pointer <= 'z') ||
                   (*input_pointer >= '0' && *input_pointer <= '9') || *input_pointer == '_') {
                input_pointer++;
            }
            strncpy(var_name, var_start, input_pointer - var_start);
            char *var_value = getenv(var_name);
            if (var_value) {
                strcat(expanded_pointer, var_value);
                expanded_pointer += strlen(var_value);
            }
        } else {
            *expanded_pointer++ = *input_pointer++;
        }
    }
    *expanded_pointer = '\0';
    strcpy(input, expanded);
}

int handle_builtin_commands(char **args) {
    if (args[0] == NULL) {return 0;}

    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(args[0], "pwd") == 0) {
        char current_dir[MAX_INPUT];
        if (getcwd(current_dir, sizeof(current_dir)) != NULL) {
            printf("%s\n", current_dir);
        } else {
            perror("pwd");
        }
        return 1;
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || chdir(args[1]) != 0) {
            perror("cd");
        }
        return 1;
    }

    if (strcmp(args[0], "set") == 0) {
        if (args[1] && args[2]) {
            if (setenv(args[1], args[2], 1) != 0) {
                perror("set");
            }
        } else {
            fprintf(stderr, "Usage: set <var name> <var value>\n");
        }
        return 1;
    }

    if (strcmp(args[0], "unset") == 0) {
        if (args[1]) {
            if (unsetenv(args[1]) != 0) {
                perror("unset");
            }
        } else {
            fprintf(stderr, "Usage: unset <var name>\n");
        }
        return 1;
    }

    return 0;
}

void execute_command(char **args, int input_fd, int output_fd, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        char *path_env = getenv("PATH");
        char cmd_path[MAX_INPUT];
        char *dir = strtok(path_env, ":");

        while (dir != NULL) {
            snprintf(cmd_path, sizeof(cmd_path), "%s/%s", dir, args[0]);
            execv(cmd_path, args);
            dir = strtok(NULL, ":");
        }

        fprintf(stderr, "%s: command not found\n", args[0]);
        exit(1);
    } else {
        if (!background) {
            waitpid(pid, NULL, 0);
        } else {
            printf("Process %d running in background\n", pid);
        }
    }
}

void handle_pipes_and_redirection(char *input) {
    char *commands[MAX_ARGS];
    int commands_num = 0;
    char *pipe_token = strtok(input, "|");

    while (pipe_token != NULL) {
        commands[commands_num++] = pipe_token;
        pipe_token = strtok(NULL, "|");
    }

    int input_fd = STDIN_FILENO, pipe_fd[2];
    for (int ix = 0; ix < commands_num; ix++) {
        char *args[MAX_ARGS];
        int output_fd = STDOUT_FILENO, background = 0;
        char *input_redir = strstr(commands[ix], "<");
        char *output_redir = strstr(commands[ix], ">");
        char *background_op = strchr(commands[ix], '&');

        if (input_redir) {
            *input_redir = '\0';
            input_redir = strtok(input_redir + 1, " \t\n");
            input_fd = open(input_redir, O_RDONLY);
            if (input_fd < 0) {
                perror("Input redirection Failed");
                return;
            }
        }
        if (output_redir) {
            *output_redir = '\0';
            output_redir = strtok(output_redir + 1, " \t\n");
            output_fd = open(output_redir, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            if (output_fd < 0) {
                perror("Output redirection Failed");
                return;
            }
        }
        if (background_op) {
            *background_op = '\0';
            background = 1;
        }

        if (ix < commands_num - 1) {
            if (pipe(pipe_fd) < 0) {
                perror("Pipe Failed");
                return;
            }
            output_fd = pipe_fd[1];
        }

        parse_input(commands[ix], args);
        if (!handle_builtin_commands(args)) {
            execute_command(args, input_fd, output_fd, background);
        }

        if (output_fd != STDOUT_FILENO) {close(output_fd);}
        if (input_fd != STDIN_FILENO) {close(input_fd);}

        if (ix < commands_num - 1) {input_fd = pipe_fd[0];}
    }
}
