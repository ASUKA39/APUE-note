#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "mytbf.h"

typedef void (*sighandler_t)(int);

struct mytbf_st
{
    int cps;
    int burst;
    int token;
    int pos;
};

static struct mytbf_st *job[MYTBF_MAX];     // job array
static volatile int inited = 0;             // init flag
static sighandler_t alrm_handler_save;      // save old signal handler, for recover if exit

static void alrm_handler(int s)             // first called by alarm in module_load(), then called by alarm in itself every one second
{
    int i;

    alarm(1);                               // one second per alarm, it constructs a loop in fact
    for(i = 0; i < MYTBF_MAX; i++)
    {
        if(job[i] != NULL)                  // if job exist, add token for all jobs
        {
            job[i]->token += job[i]->cps;
            if(job[i]->token > job[i]->burst)
                job[i]->token = job[i]->burst;
        }
    }
}

static void module_unload(void)             // if exit, recover old signal handler, cancel alarm and free all jobs
{
    int i;

    signal(SIGALRM, alrm_handler_save);     // recover old signal handler
    alarm(0);                               // cancel alarm
    for(i = 0; i < MYTBF_MAX; i++)
        free(job[i]);                       // free all jobs
}

static void module_load(void)
{
    alrm_handler_save = signal(SIGALRM, alrm_handler);  // save old signal handler, and set new signal handler
    alarm(1);                                           // set alarm, then call handler after one second
    atexit(module_unload);                              // set exit hook function
}

static int get_free_pos(void)
{
    int i;

    for(i = 0; i < MYTBF_MAX; i++)
    {
        if(job[i] == NULL)
            return i;
    }

    return -1;
}

mytbf_t *mytbf_init(int cps, int burst)
{
    struct mytbf_st *me;
    int pos;

    if(!inited)
    {
        module_load();
        inited = 1;
    }
    
    pos = get_free_pos();
    if(pos < 0)
        return NULL;

    me = malloc(sizeof(*me));               // malloc memory for single job
    if(me == NULL)
        return NULL;

    me->token = 0;
    me->cps = cps;
    me->burst = burst;
    me->pos = pos;
    job[pos] = me;

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
        return -EINVAL;                     // invalid argument

    while(me->token <= 0)
        pause();

    n = min(me->token, size);               // if token is not enough, return all token
    me->token -= n;                         // purchase token

    return n;
}

int mytbf_returntoken(mytbf_t *ptr, int size)
{
    struct mytbf_st *me = ptr;

    if(size <= 0)
        return -EINVAL;

    me->token += size;
    if(me->token > me->burst)
        me->token = me->burst;

    return size;
}

int mytbf_destroy(mytbf_t *ptr)
{
    struct mytbf_st *me = ptr;

    job[me->pos] = NULL;
    free(ptr);

    return 0;
}