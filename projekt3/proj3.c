#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define UNUSED(x) (void)(x)

enum specialCmdOpts {NONE, BCG, IN, OUT};

struct command
{
    enum specialCmdOpts special;
    char *cmd;
    char **params;
};

struct command *parseCommand(char *str)
{
    struct command *resCmd = malloc(sizeof(struct command));

    if (resCmd == NULL)
    {
        printf("malloc error: Cannot alocate new command\n");
        return NULL;
    }

    return resCmd;
}

int deleteCommand(struct command* cmd)
{
    free(cmd);
    return 0;
}

void printPrompt()
{
    printf("\n");
    printf("~$ ");
    fflush(stdout);
}

void *readThreadFunction(void *params)
{
    UNUSED(params);

    printPrompt();

    char *newLine;
    size_t newLineSize;

    while(1)
    {
        getline(&newLine, &newLineSize, stdin);
        printf("So you wanna execute %s",newLine);
        struct command *cmd = parseCommand(newLine);
        char *cmdStr[] = { "ls", "-l", (char *)0 };
        int ret = execvp ("ls", cmdStr);
        if (ret < 0) 
        {
            printf("Unable to execute given command\n");
        }
        deleteCommand(cmd);
        printPrompt();
    }
    return NULL;
}

int main(void)
{
    pthread_t readThread;
    pthread_attr_t attr;

    // Initiate attributes
    int res = pthread_attr_init(&attr);
    if (res != 0)
    {
        printf("pthread_attr error: %d\n", res);
        return EXIT_FAILURE;
    }
    if ((res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0)
    {
        printf("pthread_attr set detach error: %d\n", res);
        return EXIT_FAILURE;
    }

    // create thread
    if ((res = pthread_create(&readThread, &attr, readThreadFunction, NULL)) != 0)
    {
        printf("pthread create error: %d\n", res);
        return EXIT_FAILURE;
    }

    // join thread
    int result = -1;
    if ((res = pthread_join(readThread, (void *) &result)) != 0)
    {
        printf("pthread join error: %d\n", res);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
