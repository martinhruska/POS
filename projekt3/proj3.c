#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <wait.h>

#define UNUSED(x) (void)(x)

enum specialCmdOpts {NONE, BCG, IN, OUT};
static struct command *readCmd = NULL;

// synchronization tools
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condMonitor = PTHREAD_COND_INITIALIZER;
static int cmdLoaded = 0;


struct command
{
    enum specialCmdOpts special;
    char *cmd;
    int paramsNumber;
    char **params;
};

int findWordEnd(char *str, int start)
{
    int i = start;
    while (isspace(str[i]) != 0)
        ++i;

    while (!isspace(str[i]))
        ++i;
    return i;
}

char *getWord(char *str, int size)
{ 
    char *res = malloc((1+size)*sizeof(char));
    if (res == NULL)
    {
        printf("malloc error: Internal error\n");
        return NULL;
    }
    
    res = strncpy(res, str, size);
    res[size] = '\0';
    return res;
}

struct command *parseCommand(char *str, int size)
{
    struct command *resCmd = malloc(sizeof(struct command));

    if (resCmd == NULL)
    {
        printf("malloc error: Cannot alocate new command\n");
        return NULL;
    }
    int cend = findWordEnd(str,0);
    char *cmdName = getWord(str, cend);
    if (cmdName == NULL)
    {
        printf("malloc error: Internal error\n");
        return NULL;
    }
    resCmd->cmd = cmdName;

    int i = 0;
    int end = cend;
    while( end < size-1 )
    { // get number of params
        end = findWordEnd(str, end);
        ++i;
    }

    resCmd->params = malloc((1+i)*sizeof(char *));
    resCmd->paramsNumber = i;

    int start = 0;
    int j = 0;
    for(j=0; j<i+1; ++j)
    { // save params
        while(isspace(str[start])) // jump white spaces
            ++start;
        int pend = findWordEnd(str,start);
        resCmd->params[j] = getWord(str+start, pend-start);
        printf("added $%s$ %d %d\n", resCmd->params[j], start, pend);
        start = pend;
    }

    if (i >= 1 && strcmp(resCmd->params[i-1],"&"))
    {
        resCmd->special = BCG;
    }
    else if (i >= 1 && strcmp(resCmd->params[i-1],"<"))
    {
        resCmd->special = IN;
    }
    else if (i >= 1 && strcmp(resCmd->params[i-1],">"))
    {
        resCmd->special = OUT;
    }
    else
        resCmd->special = NONE;

    return resCmd;
}

int deleteCommand(struct command* cmd)
{
    free(cmd->cmd);
    int i = 0;
    for (i=0; i < cmd->paramsNumber; i++)
    {
        free(cmd->params[i]);
    }
    free(cmd);
    return 0;
}

void printPrompt()
{
    printf("\n");
    printf("~$ ");
    fflush(stdout);
}

void printCurrentCommad()
{
    printf("Command: %s\n",readCmd->cmd);
    printf("With params: ");
    int i = 0;
    for(i=0; i < readCmd->paramsNumber+1; i++)
    {
        printf("%s ",readCmd->params[i]);
    }
    printf("\n");
}

void *readThreadFunction(void *params)
{
    UNUSED(params);
    printPrompt();

    size_t newLineSize=513;
    char *newLine = malloc(newLineSize*sizeof(char));

    while(1)
    {
        getline(&newLine, &newLineSize, stdin);
        readCmd = parseCommand(newLine, strlen(newLine));
        printCurrentCommad();

        cmdLoaded = 1;
        pthread_cond_signal(&condMonitor);

        if (!strcmp(readCmd->cmd, "exit"))
        {
            break;
        }

        pthread_mutex_lock(&mutex);
        while(cmdLoaded)
        {
            pthread_cond_wait(&condMonitor, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        deleteCommand(readCmd);

        printPrompt();
    }
    return NULL;
}

void parentProc(pid_t child)
{
    int status = 0;
    waitpid(child, &status, 0);
    if (status != EXIT_SUCCESS)
    {
        printf("Unable to process children\n");
    }
}

void childProc()
{
    int ret = execvp (readCmd->cmd, readCmd->params);
    if (ret < 0) 
    {
        printf("Unable to execute given command\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

void executeCommand()
{   
    pid_t pid = fork();
    if (pid > 0)
    {
        parentProc(pid); 
    } 
    else if (pid == 0)
    {
        childProc();
    }
    else if (pid < 0)
    {
        perror("Fork call was unsuccesfull\n");
        exit(EXIT_FAILURE);
    }
}

void *commandThreadFunction(void *params)
{
    UNUSED(params);
    while(1)
    {
        pthread_mutex_lock(&mutex);
        while(!cmdLoaded)
        {
            pthread_cond_wait(&condMonitor, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        
        printf("Command thread is here\n");
        printCurrentCommad();
        if (!strcmp(readCmd->cmd, "exit"))
        {
            break;
        }
        executeCommand();
        cmdLoaded = 0;
        pthread_cond_signal(&condMonitor);
        fflush(stdout);
    }
    return NULL;
}


int main(void)
{
    pthread_t readThread;
    pthread_t commandThread;
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

    // create threads
    if ((res = pthread_create(&readThread, &attr, readThreadFunction, NULL)) != 0)
    {
        printf("pthread create error: %d\n", res);
        return EXIT_FAILURE;
    }
    if ((res = pthread_create(&commandThread, &attr, commandThreadFunction, NULL)) != 0)
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
    if ((res = pthread_join(commandThread, (void *) &result)) != 0)
    {
        printf("pthread join error: %d\n", res);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
