#define _POSIX_C_SOURCE 199506L
#define _XOPEN_SOURCE 500

#ifndef _REENTRANT
#define _REENTRANT

/**
 * project: Project 2 @ POS lecture
 * author: Martin Hruska
 * e-mail: xhrusk16@stud.fit.vutbr.cz
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

/*
 * Wanted critical section passes
 */
static int csPasses = 0;
/*
 * Number of currenly used ticket
 */
static int ticket = 0;

/*
 * Actual ticket to Critical Section number
 */
static int ticketToCS = 0;
/*
 * Mutex for ticket
 */
pthread_mutex_t mutexTicket=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCS=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mutexCSCond=PTHREAD_COND_INITIALIZER;

/**
 * Returns a unique ticket for entering critical section
 */
int getticket(void)
{
    pthread_mutex_lock(&mutexTicket);
    int myTicket =  ticket++;
    pthread_mutex_unlock(&mutexTicket);

    return myTicket;
}

/**
 * Enter critical section.
 * aenter - ticktet to critical section
 */
void await(int aenter)
{
    pthread_mutex_lock(&mutexCS);
    while(aenter != ticketToCS)
    {
        pthread_cond_wait(&mutexCSCond, &mutexCS);
    }
}

/**
 * Leaves critical section
 */
void advance(void)
{
    ++ticketToCS;
    pthread_cond_broadcast(&mutexCSCond);
    pthread_mutex_unlock(&mutexCS);
}

/**
 * Function of thread
 */
void *threadFunction(void *arg)
{
    const int id = *((int *) arg);
    struct timeval curTime;
    struct timespec pause;
    pause.tv_sec = 0;
    gettimeofday(&curTime, NULL);
    unsigned int seed = curTime.tv_sec*1000000+curTime.tv_usec+id;
    int myTicket = 0;

    /*
     * iterate while not enough passes
     */
    while((myTicket = getticket()) < csPasses)
    { 
        pause.tv_nsec = rand_r(&seed)%500000000L;
        nanosleep(&pause, NULL);
        await(myTicket);
        printf("%d\t(%d)\n", myTicket, id);
        advance();
        pause.tv_nsec = rand_r(&seed)%500000000L;
        nanosleep(&pause, NULL);
    }

    return 0;
}


int main(int argc, char**argv)
{
    const int threadsParam = 1;
    const int csParam = 2;
    if (argc != 3)
    {
        fprintf(stderr, "USAGE: proj02 n m\n");
        fprintf(stderr, "n ... number of threads\n");
        fprintf(stderr, "m ... number of critical section passes\n");
        return EXIT_SUCCESS;
    }

    /*
     * parse parameters
     */
    int threads = atoi(argv[threadsParam]);
    csPasses = atoi(argv[csParam]);

    pthread_t pts[threads];
    pthread_attr_t attr;

    /*
     * initiate attributes
     */
    int res = pthread_attr_init(&attr);
    if (res != 0)
    {
        fprintf(stderr,"pthread_attr error: %d\n", res);
        return EXIT_FAILURE;
    }

    if ((res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0)
    {
        fprintf(stderr,"pthread_attr set detach error: %d\n", res);
        return EXIT_FAILURE;
    }

    int ids[threads];
    int i = 0;
    /*
     * create threads
     */
    for (i=0; i < threads; ++i)
    {
        ids[i] = i+1;
        if ((res = pthread_create(&pts[i], &attr, threadFunction, (void *) &ids[i])) != 0)
        {
            fprintf(stderr,"pthread create error: %d\n", res);
            return EXIT_FAILURE;
        }
    }

    int threadsOk = 1;
    /*
     * join threads
     */
    for (i=0; i < threads; ++i)
    {
        int result = -1;
        if ((res = pthread_join(pts[i], (void *) &result)) != 0)
        {
            threadsOk = 0;
        }
    }

    if (!threadsOk)
    {
        fprintf(stderr,"pthread join error: %d\n", res);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#endif
