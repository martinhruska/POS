#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>

#define UNUSED(x) (void)(x)

enum specialCmdOpts {NONE, BCG, IN, OUT};
static struct command *readCmd = NULL;
pid_t pid=1;

/*
 * synchronization tools
 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condMonitor = PTHREAD_COND_INITIALIZER;
static int cmdLoaded = 0;
static int inter = 0;
static int jobs = 0;

/*
 * Parsed command information
 */
struct command
{
    enum specialCmdOpts special;
    char *cmd;
    int paramsNumber;
    char *output;
    char *input;
    char **params;
};

/*
 * Just print prompt
 */
void printPrompt()
{
    /*
     * printf("\n");
     */
    printf("~$ ");
    fflush(stdout);
}

void handleChld(int sig)
{
    if (sig == SIGCHLD)
    {
        int res = 0;
        pid_t pid = wait(&res);
        if (pid != -1)
        {
            --jobs;
            printf("Done \t %d\n",pid);
        }
        /*
         * printf("Child: %d ends with %d \n", pid, res);
         */
    }
}

void handleInt(int sig)
{
    if (sig == SIGINT)
    {
        if (pid == 0)
        {
            exit(EXIT_SUCCESS);
        }
        else
        {
            printf("\n");
            printPrompt();
            inter = 1;
        }
    }
}

/*
 * Find index of end of word
 */
int findWordEnd(char *str, int start)
{
    int i = start;
    while (str[i] != '\0' && isspace(str[i]) != 0)
        ++i;

    while (str[i] != '\0' && !isspace(str[i]))
        ++i;
    return i;
}

/*
 * Get word from string
 */
char *getWord(char *str, int size)
{ 
    char *res = malloc((1+size)*sizeof(char));
    if (res == NULL)
    {
        fprintf(stderr,"malloc error: Internal error\n");
        return NULL;
    }
    
    res = strncpy(res, str, size);
    res[size] = '\0';
    return res;
}

/**
 * Free alocate memory for command
 */
int deleteCommand(struct command* cmd)
{
    if (cmd == NULL)
    {
        return 0;
    }
    if (cmd->cmd != NULL)
    {
        free(cmd->cmd);
        cmd->cmd = NULL;
    }
    int i = 0;
    for (i=0; i < cmd->paramsNumber; i++)
    {
        if (cmd->params[i] != NULL)
        {
            free(cmd->params[i]);
        }
        else
        {
            break;
        }
    }

    if (cmd->output != NULL)
    {
        free(cmd->output);
    }
    if (cmd->input != NULL)
    {
        free(cmd->input);
    }

    free(cmd);
    cmd = NULL;
    return 0;
}

/*
 * Parse command from input string
 */
struct command *parseCommand(char *str, int size)
{
    struct command *resCmd = malloc(sizeof(struct command));

    if (resCmd == NULL)
    {
        fprintf(stderr, "malloc error: Cannot alocate new command\n");
        return NULL;
    }
    int cend = findWordEnd(str,0);
    char *cmdName = getWord(str, cend);
    if (cmdName == NULL)
    {
        fprintf(stderr, "malloc error: Internal error\n");
        return NULL;
    }
    resCmd->cmd = cmdName;

    int i = 0;
    int end = cend;
    /*
     * get number of params
     */
    while( end < size-1 )
    {
        end = findWordEnd(str, end);
        ++i;
    }

    /*
     * plus one is name of command, plus one is terminating string
     */
    resCmd->params = malloc((2+i)*sizeof(char *));
    resCmd->input = NULL;
    resCmd->output = NULL;
    resCmd->special = NONE;

    int start = 0;
    int j = 0;
    int specials = 0;
    enum specialCmdOpts state = NONE;
    /*
     * save params
     */
    for(j=0; j<i+1; ++j)
    {
        /*
         * jump white spaces
         */
        while(isspace(str[start]))
            ++start;
        int pend = findWordEnd(str,start);
        /*
         * printf("param: %d %d\n",start, pend);
         */
        char *temp = getWord(str+start, pend-start);

        if (!strcmp(temp,"&"))
        {
            resCmd->special = BCG;
            free(temp);
            ++specials;
        }
        else if (!strcmp(temp,"<"))
        {
            /*
             * end of params -> no input file
             */
            if (j+1 == i+1)
            {
                resCmd->params[j] = NULL;
                deleteCommand(resCmd);
                return NULL;
            }
            free(temp);
            state = IN;
            ++specials;
        }
        else if (!strcmp(temp,">"))
        {
            /*
             * end of params -> no output file
             */
            if (j+1 == i+1)
            {
                resCmd->params[j] = NULL;
                deleteCommand(resCmd);
                return NULL;
            }
            free(temp);
            state = OUT;
            ++specials;
        }
        else if (state == IN)
        {
            resCmd->input = temp;
            state = NONE;
            ++specials;
        }
        else if (state == OUT)
        {
            resCmd->output = temp;
            state = NONE;
            ++specials;
        }
        else
        {
            resCmd->params[j] = temp;
        }
        /*
         * printf("added $%s$ %d %d\n", resCmd->params[j], start, pend);
         */
        start = pend;

    }
    resCmd->paramsNumber = i-specials;
    /*
     * terminate array with null pointer
     */
    resCmd->params[resCmd->paramsNumber+1] = NULL;

    return resCmd;
}

/*
 * Print current command
 */
void printCurrentCommad()
{
    printf("Command: %s\n",readCmd->cmd);
    printf("With params: ");
    int i = 0;
    for(i=0; i < readCmd->paramsNumber+1; i++)
    {
        printf("|%s| ",readCmd->params[i]);
    }
    printf("\n");
}

int readLine(char *command, size_t max)
{
    ssize_t readChars = read(STDIN_FILENO, command, max);
    int res = 1;

    if (readChars > 512)
    {
        /*
         * skip until eol
         */
        char c;
        while (c != '\n') read(STDIN_FILENO, &c, 1); 
        fprintf(stderr,"Input is over 512 characters and that is too long\n");
        res = -1;
    }
    else if (readChars == 0)
    {
        fprintf(stderr,"EOF reached\n");
        res = 0;
    }
    else if (readChars < 0)
    {
        fprintf(stderr,"Some error on reading input\n");
        res = -1;
    }
    else
    {
        command[readChars-1] = '\0';
    }

    return res;
}

/**
 * Function for thread reading commands
 */
void *readThreadFunction(void *params)
{
    UNUSED(params);
    printPrompt();

    size_t newLineSize=513;

    while(1)
    {
        char *newLine = malloc(newLineSize*sizeof(char));
        memset(newLine,'\0',newLineSize);
        /*
         * getline(&newLine, &newLineSize, stdin);
         */
        int resRead = readLine(newLine, newLineSize);
        if (resRead == 0)
        {
            free(newLine);
            deleteCommand(readCmd);
            break;
        }
        /*
         * read failed or no command given
         */
        else if (resRead < 0 || strlen(newLine) == 0)
        {
            free(newLine);
            printPrompt();
            continue;
        }
        /*
         * printf("Read: %s %d\n",newLine, strlen(newLine));
         */
        readCmd = parseCommand(newLine, strlen(newLine));
        free(newLine);
        if (readCmd == NULL)
        {
            printPrompt();
            continue;
        }
        /*
         * printCurrentCommad();
         * printf("params: %d\n", readCmd->paramsNumber);
         */

        cmdLoaded = 1;
        pthread_cond_signal(&condMonitor);

        if (!strcmp(readCmd->cmd, "exit"))
        {
            break;
        }

        /*
         * interupted before waiting
         */
        inter = 0;
        pthread_mutex_lock(&mutex);
        while(cmdLoaded)
        {
            pthread_cond_wait(&condMonitor, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        deleteCommand(readCmd);

        /*
         * if there were no interuptions, print prompt
         */
        if (!inter)
        {
            printPrompt();
        }
    }
    return NULL;
}

void parentProc(pid_t child)
{
    int status = 0;
    if (readCmd->special != BCG)
    {
        waitpid(child, &status, 0);
    }
    else
    {
        ++jobs;
        printf("[%d] %d\n",jobs, child);
    }
}

int prepareOut()
{
    if (readCmd == NULL)
    {
        return 0;
    }

    /*
     * stdout, nothing to do
     */
    if (readCmd->output == NULL)
    {
        return 0;
    }

    int newOut = open(readCmd->output, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (newOut == -1)
    {
        return -1;
    }
    /*
     * close stdout
     */
    close(STDOUT_FILENO);
    /*
     * create a new fileno
     */
    dup2(newOut, STDOUT_FILENO);
    return 0;
}

int prepareIn()
{
    if (readCmd == NULL)
    {
        return 0;
    }

    /*
     * stdout, nothing to do
     */
    if (readCmd->input == NULL)
    { 
        return 0;
    }

    int newOut = open(readCmd->input, O_CREAT | O_RDONLY);
    if (newOut == -1)
    {
        return -1;
    }
    /*
     * close stdout
     */
    close(STDIN_FILENO);
    /*
     * create a new fileno
     */
    dup2(newOut, STDIN_FILENO);
    return 0;
}

int childProc()
{
    if (readCmd == NULL)
    {
        return EXIT_SUCCESS;
    }
    if (prepareOut() < 0 )
    {
        printf("Unable to open output: %s", readCmd->output);
        return EXIT_FAILURE;
    }
    if (prepareIn() < 0 )
    {
        printf("Unable to input output: %s", readCmd->input);
        return EXIT_FAILURE;
    }
    int ret = execvp (readCmd->cmd, readCmd->params);
    if (ret < 0) 
    {
        fprintf(stderr, "Unable to execute given command: %s\n", readCmd->cmd);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Execution of command
 */
void executeCommand()
{   
    pid = fork();
    if (pid > 0)
    {
        parentProc(pid); 
    } 
    else if (pid == 0)
    {
        int res = childProc();
        exit(res);
    }
    else if (pid < 0)
    {
        perror("Fork call was unsuccesfull\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Function for thread which executes commands
 */
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
       
        /*
         * printf("Command thread is here\n");
         * printCurrentCommad();
         */
        if (!strcmp(readCmd->cmd, "exit"))
        {
            deleteCommand(readCmd);
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
    struct sigaction sigact;
    struct sigaction sigchld;
    sigset_t setint;
    sigset_t setchld;

    
    sigemptyset(&setint);
    sigaddset(&setint, SIGINT);
    
    sigemptyset(&setchld);
    sigaddset(&setchld, SIGCHLD);

    /*
     * Init my own handlers
     */
    sigprocmask(SIG_BLOCK, &setint, NULL);

    /*
     * catch sigint signal
     */
    sigact.sa_handler = handleInt;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if (sigaction(SIGINT, &sigact, NULL) == -1)
    {
        return 1;
    }

    /*
     * catch sigchld signal
     */
    sigchld.sa_handler = handleChld;
    sigemptyset(&sigchld.sa_mask);
    sigchld.sa_flags = 0;
    if (sigaction(SIGCHLD, &sigchld, NULL) == -1)
    {
        return 1;
    }
    
    sigprocmask(SIG_UNBLOCK, &setint, NULL);

    pthread_t readThread;
    pthread_t commandThread;
    pthread_attr_t attr;

    /*
     * Initiate attributes
     */
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

    /*
     * create threads
     */
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

    /*
     * join thread
     */
    int result = -1;
    if ((res = pthread_join(readThread, (void *) &result)) != 0)
    {
        printf("pthread join error: %d\n", res);
        return EXIT_FAILURE;
    }
    if ((res = pthread_join(commandThread, (void *) &result)) != 0)
    {
        fprintf(stderr,"pthread join error: %d\n", res);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}