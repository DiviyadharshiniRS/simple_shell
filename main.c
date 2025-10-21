#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024

int main() {
    char input[MAX_INPUT];
    char *args[100];
    char *token;
    int status = 1;

    printf("SimpleShell v1.0 — by Team Diviy\n");

    while (status) {
        printf("shell> ");
        fflush(stdout);

        // Read input from user
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; // Exit on Ctrl+D
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        // Exit command
        if (strcmp(input, "exit") == 0)
            break;

        // Tokenize input into arguments
        int i = 0;
        token = strtok(input, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Fork a child process
        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1)
                perror("shell");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // Parent waits for child
            wait(NULL);
        } else {
            perror("fork failed");
        }
    }

    printf("Exiting shell... Goodbye!\n");
    return 0;
}
