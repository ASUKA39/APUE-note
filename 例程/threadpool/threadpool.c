#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define LEFT 30000000
#define RIGHT 30020000
#define THRNUM 8                                // not the more the better, it depends on the CPU cores

static int num = 0;                             // the number to be judged, 0 means no number, -1 means all tasks are done and threads can exit
static pthread_mutex_t mut_num = PTHREAD_MUTEX_INITIALIZER;     // mutex lock for num

static void *thr_prime(void *p)
{
    int i, j, mark;

    while (1)
    {
        // critical section begin
        pthread_mutex_lock(&mut_num);
        while (num == 0)                        // no number to be judged
        {
            pthread_mutex_unlock(&mut_num);
            sched_yield();
            pthread_mutex_lock(&mut_num);
        }
        if (num == -1)
        {
            pthread_mutex_unlock(&mut_num);
            break;                              // break the big while and exit until all tasks are done
        }
        i = num;
        num = 0;
        pthread_mutex_unlock(&mut_num);         // critical section end

        mark = 1;
        for (j = 2; j < i/2; j++)
        {
            if (i % j == 0)
            {
                mark = 0;
                break;
            }
        }
        if (mark)
        {
            printf("[%d] %d is a primer\n", (int)p, i);
        }
    }
    pthread_exit(NULL);                         // thread exit but not terminate
}

int main()
{
    int i, err;
    pthread_t tid[THRNUM];                      // thread pool

    for (i = 0; i < THRNUM; i++)
    {
        err = pthread_create(tid+i, NULL, thr_prime, (void *)i);
        if (err)
        {
            fprintf(stderr, "pthread_create(): %s\n", strerror(err));
            exit(1);
        }
    }

    for(i = LEFT; i <= RIGHT; i++)
    {
        // critical section begin
        pthread_mutex_lock(&mut_num);
        while (num != 0)                        // num == 0 means there has been a thread to take the task
        {
            pthread_mutex_unlock(&mut_num);
            sched_yield();                      // unlock and wait for a thread to take the task
            pthread_mutex_lock(&mut_num);
        }
        num = i;                                // now there is a thread to take the task and we can assign a new task
        pthread_mutex_unlock(&mut_num);         // critical section end
    }

    // wait for all tasks to be done
    // critical section begin
    pthread_mutex_lock(&mut_num);
    while (num != 0)                            // num != 0 means there is still a task to be done while main thread has no task to assign
    {
        pthread_mutex_unlock(&mut_num);
        sched_yield();
        pthread_mutex_lock(&mut_num);
    }
    num = -1;                                   // no task to assign and all tasks are done, so we can set num to -1 to call all threads to exit
    pthread_mutex_unlock(&mut_num);             // critical section end

    for (i = 0; i < THRNUM; i++)
    {
        pthread_join(tid[i], NULL);             // wait for all threads to exit
    }

    pthread_mutex_destroy(&mut_num);

    return 0;
}