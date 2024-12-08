#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "shell.h"

#define MAX_INPUT 1024
#define MAX_ARGS 100

void parse_input(char *input, char **args);

void expand_var(char *input);

int handle_builtin_commands(char **args);

void execute_command(char **args, int input_fd, int output_fd, int background);

void handle_pipes_and_redirection(char *input);


#endif