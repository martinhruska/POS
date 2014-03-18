#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

// Wanted critical section passes
static int csPasses = 0;
// Number of currenly used ticket
static int ticket = 0;

static int ticketToCS = 1;
// Mutex for ticket
pthread_mutex_t mutexTicket=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCS=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mutexCSCond=PTHREAD_COND_INITIALIZER;

/**
 * Returns a unique ticket for entering critical section
 */
int getticket(void)
{
    int res = pthread_mutex_lock(&mutexTicket);
    if (res != 0)
    {
        printf("mutex_lock error %d", res);
        return EXIT_FAILURE;
    }

    int myTicket =  ++ticket;
    res = pthread_mutex_unlock(&mutexTicket);

    if (res != 0)
    {
        printf("mutex_unlock error %d", res);
        return EXIT_FAILURE;
    }
    return myTicket;
}

/**
 * Enter critical section.
 * aenter - ticktet to critical section
 */
void await(int aenter)
{
    //printf("going to wait for %d == %d\n", aenter, ticketToCS);
    pthread_mutex_lock(&mutexCS);
    //printf("waiting for %d == %d\n", aenter, ticketToCS);
    while(aenter != ticketToCS)
    {
        pthread_cond_wait(&mutexCSCond, &mutexCS);
        //printf("another cycle for %d == %d\n", aenter, ticketToCS);
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

    while((myTicket = getticket()) <= csPasses)
    {
        pause.tv_nsec = rand_r(&seed)%500000000L;
        //printf("going to sleep with ticket %d for %d\n",myTicket, pause.tv_nsec);
        nanosleep(&pause, NULL);
        await(myTicket);
        //printf("awaited %d\n", myTicket);
        printf("%d\t(%d)\n", myTicket, id);
        advance();
        pause.tv_nsec = rand_r(&seed)%500000000L;
        //printf("adavanced %d\n", myTicket);
        //printf("going to sleep for %d\n",pause.tv_nsec);
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
        perror("Wrong numbers of the parameters\n");
        return EXIT_FAILURE;
    }

    int threads = atoi(argv[threadsParam]);
    csPasses = atoi(argv[csParam]);

    pthread_t pts[threads];
    pthread_attr_t attr;

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

    int ids[threads];
    int i = 0;
    for (i=0; i < threads; ++i)
    {
        ids[i] = i+1;
        if ((res = pthread_create(&pts[i], &attr, threadFunction, (void *) &ids[i])) != 0)
        {
         printf("pthread create error: %d\n", res);
         return EXIT_FAILURE;
        }
    }

    int threadsOk = 1;
    for (i=0; i < threads; ++i)
    {
        int result = -1;
        if ((res = pthread_join(pts[i], (void *) &result)) != 0)
        {
            threadsOk = 0;
        }
        //printf("thread ended with state: %d\n", result);
    }

    if (!threadsOk)
    {
        printf("pthread join error: %d\n", res);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
