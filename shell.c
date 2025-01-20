#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_LENGTH 255    // Max length for input commands

char prev_command[MAX_INPUT_LENGTH];  // Stores the previous command

#include "tokenize_build.h"

// Function prototypes for shell operations
int handle_builtin_commands(char **args);
int execute_command(char **args);
void handle_pipes(char **args1, char **args2);
void handle_redirection(char **args);
void free_tokens(char **tokens);
void execute_sequential_commands(char *input);

// Handles built-in commands like cd, prev, source, help
int handle_builtin_commands(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            char *home = getenv("HOME");
            if (home != NULL) {
                if (chdir(home) != 0) {
                    perror("cd");
                }
            }
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }

    if (strcmp(args[0], "prev") == 0) {
        if (strlen(prev_command) > 0) {
            printf("Previous command: %s\n", prev_command);
            char **tokens = tokenize(prev_command);
            int result = execute_command(tokens);
            free_tokens(tokens);
            return result;
        }
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("Supported built-in commands: cd, pwd, prev, source, help, exit\n");
        return 1;
    }

    if (strcmp(args[0], "source") == 0) {
        if (args[1] == NULL) {
            printf("Expected filename for \"source\"\n");
            return 1;
        }
        
        int fd = open(args[1], O_RDONLY);
        if (fd == -1) {
            printf("source: Could not open file\n");
            return 1;
        }

        char buffer[MAX_INPUT_LENGTH];
        int bytes_read;
        int pos = 0;

        while ((bytes_read = read(fd, &buffer[pos], 1)) > 0) {
            if (buffer[pos] == '\n' || pos == MAX_INPUT_LENGTH - 1) {
                buffer[pos] = '\0';
                if (strlen(buffer) > 0) {
                    char **tokens = tokenize(buffer);
                    if (tokens[0] != NULL && strcmp(tokens[0], "exit") != 0) {
                        execute_command(tokens);
                    }
                    free_tokens(tokens);
                }
                pos = 0;
            } else {
                pos++;
            }
        }

        // Handle last line if it doesn't end with newline
        if (pos > 0) {
            buffer[pos] = '\0';
            if (strlen(buffer) > 0) {
                char **tokens = tokenize(buffer);
                if (tokens[0] != NULL && strcmp(tokens[0], "exit") != 0) {
                    execute_command(tokens);
                }
                free_tokens(tokens);
            }
        }

        close(fd);
        return 1;
    }
    return 0;
}

// Updates the previous command buffer
void update_prev_command(const char *input) {
    if (strcmp(input, "") != 0) {
        strncpy(prev_command, input, MAX_INPUT_LENGTH - 1);
        prev_command[MAX_INPUT_LENGTH - 1] = '\0';
    }
}

// Handles input/output redirection for commands
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (args[i + 1] != NULL) {
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[i] = NULL;
            }
        } else if (strcmp(args[i], "<") == 0) {
            if (args[i + 1] != NULL) {
                int fd = open(args[i + 1], O_RDONLY);
                if (fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
                args[i] = NULL;
            }
        }
    }
}

// Handles piping between two commands
void handle_pipes(char **args1, char **args2) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        handle_redirection(args1);
        execvp(args1[0], args1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        handle_redirection(args2);
        execvp(args2[0], args2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Executes a single command
int execute_command(char **args) {
    if (args[0] == NULL) return 0;
    
    if (handle_builtin_commands(args)) {
        return 0;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        handle_redirection(args);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

// Frees memory allocated for tokens
void free_tokens(char **tokens) {
    if (tokens == NULL) return;
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// Executes multiple commands separated by semicolons
void execute_sequential_commands(char *input) {
    char *command = strtok(input, ";");
    
    while (command != NULL) {
        while (*command == ' ') command++;
        
        if (strlen(command) > 0) {
            char **tokens = tokenize(command);
            if (tokens[0] != NULL) {
                execute_command(tokens);
            }
            free_tokens(tokens);
        }
        command = strtok(NULL, ";");
    }
}

int main(int argc, char **argv) {
    char input[MAX_INPUT_LENGTH];
    prev_command[0] = '\0';
    
    printf("Welcome to mini-shell.\n");

    while (1) {
        printf("shell $ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) {
            printf("Bye bye.\n");
            break;
        }

        if (strchr(input, ';') != NULL) {
            execute_sequential_commands(input);
        } else if (strchr(input, '|') != NULL) {
            char *command1 = strtok(input, "|");
            char *command2 = strtok(NULL, "|");
            if (command1 != NULL && command2 != NULL) {
                while (*command2 == ' ') command2++;
                
                char **tokens1 = tokenize(command1);
                char **tokens2 = tokenize(command2);
                if (tokens1[0] != NULL && tokens2[0] != NULL) {
                    handle_pipes(tokens1, tokens2);
                }
                free_tokens(tokens1);
                free_tokens(tokens2);
            }
        } else {
            char **tokens = tokenize(input);
            if (tokens[0] != NULL) {
                execute_command(tokens);
            }
            free_tokens(tokens);
        }

        if (strlen(input) > 0) {
            update_prev_command(input);
        }
    }

    return 0;
}