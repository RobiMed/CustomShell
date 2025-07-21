#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT 1024 // Maximum input buffer size
#define MAX_PROCESSES 64 // Maximum number of background processes

// Array to keep track of background processes
pid_t background_processes[MAX_PROCESSES];
int process_count = 0;

// Function prototypes
void execute_command(char *input);
void execute_program(char *cmd, int background, char *output_file);
void print_globalusage();
void quit_shell();
void check_background_processes();

int main() {
    char input[MAX_INPUT];
    
    while (1) {
        printf("user@host> ");  // Display shell prompt
        fflush(stdout); // Ensure output is printed immediately
        
        if (!fgets(input, MAX_INPUT, stdin)) {  // Read user input
            break;
        }
        
        input[strcspn(input, "\n")] = 0; // Remove trailing newline character
        execute_command(input); // Process the input command
    }
    return 0;
}

// Function to parse and execute a command
void execute_command(char *input) {
    if (strlen(input) == 0) return;
    
    char *cmd = strtok(input, " ");
    
    if (strcmp(cmd, "exec") == 0) {
        char *args = strtok(NULL, "");
        int background = 0;
        char *output_file = NULL;
        
        if (args) {
            char *ampersand = strstr(args, "&");
            if (ampersand) {
                background = 1;
                *ampersand = '\0';
            }
            
            char *redir = strstr(args, ">");
            if (redir) {
                *redir = '\0';
                output_file = strtok(redir + 1, " ");
            }
        }
        
        execute_program(args, background, output_file);
    } 
    else if (strcmp(cmd, "globalusage") == 0) {
        print_globalusage();
    } 
    else if (strcmp(cmd, "quit") == 0) {
        quit_shell();
    } 
    else {
        printf("Unknown command: %s\n", cmd);
    }
}

// Function to execute a program
void execute_program(char *cmd, int background, char *output_file) {
    if (!cmd) {
        printf("No command provided.\n");
        return;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        if (output_file) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        char *argv[64];
        int i = 0;
        char *token = strtok(cmd, " ");
        while (token && i < 63) {
            argv[i++] = token;
            token = strtok(NULL, " ");
        }
        argv[i] = NULL;
        
        execvp(argv[0], argv);
        perror("exec");
        exit(1);
    } else if (pid > 0) {
        if (background) {
            if (process_count < MAX_PROCESSES) {
                background_processes[process_count++] = pid;
            }
            printf("Background process started: %d\n", pid);
        } else {
            waitpid(pid, NULL, 0);
            printf("Process finished: %d\n", pid);
        }
    } else {
        perror("fork");
    }
}

// Function to check background processes
void check_background_processes() {
    int active_processes = 0;
    for (int i = 0; i < process_count; i++) {
        if (waitpid(background_processes[i], NULL, WNOHANG) == 0) {
            if (active_processes == 0) {
                printf("The following processes are running:\n");
            }
            printf("PID: %d\n", background_processes[i]);
            active_processes++;
        }
    }
    if (active_processes > 0) {
        printf("Are you sure you want to quit? [Y/n] ");
        char response;
        scanf(" %c", &response);
        if (response != 'Y' && response != 'y') {
            return;
        }
    }
}

// Function to handle quitting the shell
void quit_shell() {
    check_background_processes();
    printf("Quitting IMCSH...\n");
    exit(0);
}

// Function to print global usage information
void print_globalusage() {
    printf("IMCSH Version 1.1 created by Robert Medvešek & Xander Smakman\n");
}
