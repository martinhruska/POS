#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

int sEnd=0;
pid_t pid=-1;
char outputChar = 'A';

sig_atomic_t sigUsr1 = 0;

void handleInt(int sig)
{
    if (pid == 0)
    {
        sEnd = 1;
    }
    else if (pid > 0)
    { // TODO is it neccessary to have all this in signal handler
        int status = 0;
        kill(pid, SIGUSR1);
        waitpid(pid, &status, 0);
        if (status != EXIT_SUCCESS)
        {
            perror("Child ends with error\n");
            exit(EXIT_FAILURE);
        }
        exit(0);
    }
}

void handleUsr1(int sig)
{
    sigUsr1 = 1;
}

void handleUsr2(int sig)
{
    printf("handle 2\n");
    outputChar = 'A';
}

int parrentProc(pid_t childPid, pid_t myPid, sigset_t* setusr)
{
    sigset_t emptySet;
    while(1)
    {
        sigprocmask(SIG_BLOCK, setusr, NULL);
        sigUsr1 = 0;
        printf("Parent (%d): '%c'\n", myPid, outputChar);
        if (outputChar == 'Z')
        {
            outputChar = 'A';
        }
        else
        {
            outputChar += 1;
        }
        kill(childPid, SIGUSR1);
        while(!sigUsr1)
        {
            sigsuspend(&emptySet);
        }
        sigprocmask(SIG_UNBLOCK, setusr, NULL);
        printf("Press enter...\n");
        int input = getchar();
        while (input != '\n')
        {
            input = getchar();
        }
    }
    return 0;
}

int childProc(pid_t parentPid, pid_t myPid, sigset_t* setusr)
{
    sigset_t emptySet;
    while(!sEnd)
    {
        sigprocmask(SIG_BLOCK, setusr, NULL);
        while(!sigUsr1)
        {
            sigsuspend(&emptySet);
        }

        if (!sEnd)
            printf("Child (%d): '%c'\n", myPid, outputChar);
        else
            break;
        if (outputChar == 'Z')
        {
            outputChar = 'A';
        }
        else
        {
            outputChar += 1;
        }
        kill(parentPid, SIGUSR1);
        sigUsr1 = 0; // wait for parent
        sigprocmask(SIG_UNBLOCK, setusr, NULL);
    }
    kill(parentPid, SIGUSR1);
    return 0;
}

int main(void)
{

    struct sigaction sigact;
    struct sigaction sigusr1;
    struct sigaction sigusr2;
    sigset_t setint;
    sigset_t setusr;

    sigemptyset(&setint);
    sigaddset(&setint, SIGINT);
    
    sigemptyset(&setusr);
    sigaddset(&setusr, SIGUSR1);
    sigaddset(&setusr, SIGUSR2);

    // Init my own handlers
    sigprocmask(SIG_BLOCK, &setint, NULL);

    sigact.sa_handler = handleInt;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if (sigaction(SIGINT, &sigact, NULL) == -1)
    {
        return 1;
    }

    sigusr1.sa_handler = handleUsr1;
    sigemptyset(&sigusr1.sa_mask);
    sigusr1.sa_flags = 0;
    if (sigaction(SIGUSR1, &sigusr1, NULL) == -1)
    {
        return 1;
    }

    sigusr2.sa_handler = handleUsr2;
    sigemptyset(&sigusr2.sa_mask);
    sigusr2.sa_flags = 0;
    if (sigaction(SIGUSR2, &sigusr2, NULL) == -1)
    {
        return 1;
    }
    
    sigprocmask(SIG_UNBLOCK, &setint, NULL);

    pid = fork();
    if (pid > 0)
    {
        parrentProc(pid, getpid(), &setusr); 
    } 
    else if (pid == 0)
    {
        childProc(getppid(), getpid(), &setusr);
    }
    else if (pid < 0)
    {
        perror("Fork  call was unsuccesfull\n");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
