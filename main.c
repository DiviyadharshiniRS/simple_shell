#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_INPUT 1024

// Trim leading and trailing spaces
void trim(char *str) {
    int i, j;
    for(i = 0; str[i] == ' '; i++); // skip leading spaces
    int len = strlen(str);
    for(j = 0; len - j > 0 && str[len - 1 - j] == ' '; j++); // skip trailing
    memmove(str, str + i, len - i - j);
    str[len - i - j] = '\0';
}

// Preprocess input: add spaces around <, >, >> only if present
void preprocess_input(char *input) {
    char buffer[MAX_INPUT];
    int j = 0;
    int len = strlen(input);

    for (int i = 0; i < len; i++) {
        if (input[i] == '>') {
            if (i + 1 < len && input[i + 1] == '>') {  // handle '>>'
                if (j > 0 && buffer[j - 1] != ' ') buffer[j++] = ' ';
                buffer[j++] = '>';
                buffer[j++] = '>';
                i++;
                if (i + 1 < len && input[i + 1] != ' ') buffer[j++] = ' ';
            } else {  // single '>'
                if (j > 0 && buffer[j - 1] != ' ') buffer[j++] = ' ';
                buffer[j++] = '>';
                if (i + 1 < len && input[i + 1] != ' ') buffer[j++] = ' ';
            }
        } else if (input[i] == '<') {
            if (j > 0 && buffer[j - 1] != ' ') buffer[j++] = ' ';
            buffer[j++] = '<';
            if (i + 1 < len && input[i + 1] != ' ') buffer[j++] = ' ';
        } else {
            buffer[j++] = input[i];
        }
    }
    buffer[j] = '\0';
    strcpy(input, buffer);
}

// Remove quotes from a string
void remove_quotes(char *str) {
    int i, j = 0;
    char temp[MAX_INPUT];
    for (i = 0; str[i]; i++) {
        if (str[i] != '"') temp[j++] = str[i];
    }
    temp[j] = '\0';
    strcpy(str, temp);
}

// Tokenize input into args[], handle redirection and background
int tokenize_input(char *input, char **args, int *background,
                   char **input_file, char **output_file, int *append_out) {
    int i = 0;
    int in_quote = 0;
    char *p = input;
    char token[MAX_INPUT];
    int t_index = 0;

    while (*p) {
        if (*p == '"') { in_quote = !in_quote; p++; continue; }

        if (!in_quote && (*p == ' ' || *p == '\t')) {
            if (t_index > 0) {
                token[t_index] = '\0';
                args[i++] = strdup(token);
                t_index = 0;
            }
            p++;
        } else if (!in_quote && *p == '&') {
            *background = 1;
            p++;
        } else if (!in_quote && *p == '<') {
            p++;
            while (*p == ' ') p++;
            char *fname = strtok(p, " \n");
            *input_file = fname;
            p += strlen(fname);
        } else if (!in_quote && *p == '>') {
            if (*(p + 1) == '>') {
                *append_out = 1;
                p += 2;
            } else {
                *append_out = 0;
                p++;
            }
            while (*p == ' ') p++;
            char *fname = strtok(p, " \n");
            *output_file = fname;
            p += strlen(fname);
        } else {
            token[t_index++] = *p;
            p++;
        }
    }

    if (t_index > 0) {
        token[t_index] = '\0';
        args[i++] = strdup(token);
    }

    args[i] = NULL;
    return i;
}

void execute(char *input) {
    char *args[100];
    int background = 0;
    char *input_file = NULL;
    char *output_file = NULL;
    int append_out = 0;

    preprocess_input(input);
    trim(input);
    tokenize_input(input, args, &background, &input_file, &output_file, &append_out);

    if (args[0] == NULL) return;

    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell... Goodbye!\n");
        exit(0);
    }

    // Remove quotes from all args
    for (int k = 0; args[k]; k++) remove_quotes(args[k]);

    pid_t pid = fork();
    if (pid < 0) { perror("fork failed"); return; }

    if (pid == 0) {
        // Child process: handle I/O redirection
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) { perror("input file"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (output_file) {
            int fd;
            if (append_out)
                fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            else
                fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0) { perror("output file"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } else {
        // Parent process
        if (!background)
            waitpid(pid, NULL, 0);
        else
            printf("[Background] PID: %d started\n", pid);
    }

    // Free strdup() memory
    for (int k = 0; args[k]; k++) free(args[k]);
}

int main() {
    char input[MAX_INPUT];

    printf("\n🔹 Simple Shell v3.3 — by Team Diviya\n");
    printf("Type commands, use '&' for background, 'exit' to quit.\n\n");

    while (1) {
        int status;
        pid_t done;

        // Check for finished background processes
        while ((done = waitpid(-1, &status, WNOHANG)) > 0) {
            printf("[Background] PID %d finished.\n", done);
        }

        printf("myshell> ");
        fflush(stdout);

        memset(input, 0, sizeof(input)); // Clear input buffer
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = '\0';

        execute(input);
    }

    return 0;
}
