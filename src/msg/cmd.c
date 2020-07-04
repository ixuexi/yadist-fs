#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int option_vec_fill(char *argv[], int size, char *opts)
{
    char *str = opts;
    char *end;
    int argc = 1;

    while (argc < size - 1) {
        while (*str == ' ') str++;
        end = strstr(str, " ");
        if (end) {
            argv[argc++] = strndup(str, end - str);
            str = end;
            continue;
        }
        else {
            argv[argc++] = strdup(str);
            break;
        }
    }
    argv[argc++] = NULL;

    return argc;
}

void option_vec_free(char *argv[], int size)
{
    int i;
    for (i = 1; i < size; i++)
        if (argv[i]) {
            free(argv[i]);
            argv[i] = NULL;
        }
}

void option_vec_print(char *argv[], int size)
{
    int i;
    printf("options: ");
    for (i = 1; i < size; i++)
        if (argv[i]) {
            printf("[%s] ", argv[i]);
        }
    printf("\n");
}

int execute_command(char *cmd, char *argv[])
{
    pid_t pid = fork();
    int status;

    if (pid != 0) {
        waitpid(pid, &status, 0);
        return status;
    }
    else {
        execvp(cmd, argv);
        return 0;
    }
}
