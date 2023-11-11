#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#include "mytbf.h"

struct mytbf_st
{
    int cps;
    int burst;
    int token;
    int pos;
    pthread_mutex_t mut;
};

static struct mytbf_st *job[MYTBF_MAX];
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER;
static pthread_t tid_alrm;                                  // used for alarm thread
static pthread_once_t init_once = PTHREAD_ONCE_INIT;        // used for init once
static volatile int inited = 0;

static void *thr_alrm(void *p)                              // alarm thread, called by pthread_create
{
    int i;

    while(1)
    {
        pthread_mutex_lock(&mut_job);                       // lock the job array
        for(i = 0; i < MYTBF_MAX; i++)
        {
            if(job[i] != NULL)
            {
                pthread_mutex_lock(&job[i]->mut);           // lock the job[i]
                job[i]->token += job[i]->cps;
                if(job[i]->token > job[i]->burst)
                    job[i]->token = job[i]->burst;
                pthread_mutex_unlock(&job[i]->mut);
            }
        }
        pthread_mutex_unlock(&mut_job);
        sleep(1);
    }
}

static void module_unload(void)
{
    int i;

    pthread_cancel(tid_alrm);                               // cancel the alarm thread
    pthread_join(tid_alrm, NULL);                           // wait for the alarm thread to exit
    for(i = 0; i < MYTBF_MAX; i++)
    {
        if(job[i] != NULL)
        {
            pthread_mutex_destroy(&job[i]->mut);
            free(job[i]);
        }
    }
    pthread_mutex_destroy(&mut_job);
}

static void module_load(void)
{
    pthread_t tid_alrm;
    int err;
    pthread_create(&tid_alrm, NULL, thr_alrm, NULL);        // create the alarm thread
    if(err)
    {
        fprintf(stderr, "pthread_create(): %s\n", strerror(err));
        exit(1);
    }
    
    atexit(module_unload);
}

static int get_free_pos_unlocked(void)                      // unlocked version of get_free_pos, plz lock the mut_job before calling this function
{
    int i;

    for(i = 0; i < MYTBF_MAX; i++)
    {
        if(job[i] == NULL)
            return i;
    }
    return -1;
}

mytbf_t *mytbf_init(int cps, int burst)                     // called by user
{
    struct mytbf_st *me;
    int pos;

    pthread_once(&init_once, module_load);                  // init the module once
    
    me = malloc(sizeof(*me));
    if(me == NULL)
        return NULL;
    me->token = 0;
    me->cps = cps;
    me->burst = burst;
    pthread_mutex_init(&me->mut, NULL);                     // init the mutex of me

    pthread_mutex_lock(&mut_job);
    pos = get_free_pos_unlocked();
    if(pos < 0){
        pthread_mutex_unlock(&mut_job);
        free(me);
        return NULL;
    }
    me->pos = pos;
    job[pos] = me;
    pthread_mutex_unlock(&mut_job);

    return me;
}

static int min(int a, int b)
{
    if(a < b)
        return a;
    return b;
}

int mytbf_fetchtoken(mytbf_t *ptr, int size)
{
    struct mytbf_st *me = ptr;
    int n;

    if(size <= 0)
        return -EINVAL;

    pthread_mutex_lock(&me->mut);
    while(me->token <= 0)
    {
        pthread_mutex_unlock(&me->mut);
        sched_yield();                                      // wait for the token from alarm thread
        pthread_mutex_lock(&me->mut);
    }

    n = min(me->token, size);
    me->token -= n;
    pthread_mutex_unlock(&me->mut);

    return n;
}

int mytbf_returntoken(mytbf_t *ptr, int size)
{
    struct mytbf_st *me = ptr;

    if(size <= 0)
        return -EINVAL;

    pthread_mutex_lock(&me->mut);
    me->token += size;
    if(me->token > me->burst)
        me->token = me->burst;
    pthread_mutex_unlock(&me->mut);

    return size;
}

int mytbf_destroy(mytbf_t *ptr)
{
    struct mytbf_st *me = ptr;
    
    pthread_mutex_lock(&mut_job);
    job[me->pos] = NULL;
    pthread_mutex_unlock(&mut_job);
    pthread_mutex_destroy(&me->mut);
    free(ptr);

    return 0;
}