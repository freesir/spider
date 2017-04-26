#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "threadpool.h"

static char* thread_state_map[] ={
    "create", 
    "waiting", 
    "proccess", 
    "done", 
    "exit"
};

#define THREAD_BUSY_PERCENT  0.5
#define THREAD_IDLE_PERCENT  2

static void *thread_excute_route(void *arg);
static void *display_thread(void *arg);
static void *thread_pool_is_need_recovery(void *arg);
static void *thread_pool_is_need_extend(void *arg);

thread_pool_t *
thread_pool_create(int num)
{
    thread_pool_t *p = NULL ;
    thread_info_t *tmp  = NULL;
    int i = 0;

    if(num < 1)
    {
        return p;
    }
    
    p = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if(p == NULL)
    {
        return NULL;
    }

    p->init_num = num;
    
    pthread_mutex_init(&(p->queue_lock), NULL);
    pthread_cond_init(&(p->queue_ready), NULL);

    p->num = num; 
    p->rnum = num;
    p->knum = 0;

    p->head = NULL;
    p->queue_size = 0; 
    p->is_destroy = false;

    for(i = 0; i < num; i++)
    {
        tmp = (thread_info_t*)malloc(sizeof(thread_info_t));
        if(tmp==NULL)
        {
             free(p);
             return NULL;
        }
        else
        {
            tmp->next = p->threads;
            p->threads = tmp;
        }
        
        pthread_create(&(tmp->id), NULL, thread_excute_route, p);
        tmp->state = THREAD_STATE_RUN;
    }

    //pthread_create(&(p->display), NULL, display_thread, p);
    pthread_create(&(p->destroy), NULL, thread_pool_is_need_recovery, p);
    
    return p;
}

int 
thread_pool_add_worker(thread_pool_t *pool, void*(*process)(void*arg), void*arg)
{
    thread_pool_t *p = NULL;
    thread_worker_t *worker = NULL, *member = NULL;

    assert(pool);
    assert(arg);

    p = pool;
    
    worker = (thread_worker_t*)malloc(sizeof(thread_worker_t));
    if(worker == NULL)
    {
        return -1;
    }
    
    worker->process = process;
    worker->arg = arg;
    worker->next = NULL;
    thread_pool_is_need_extend(pool);
    
    pthread_mutex_lock(&(p->queue_lock));
    member = p->head;
    if(member!=NULL)
    {
        while(member->next!=NULL)
        {
            member = member->next;
        }
        
        member->next = worker;
    }
    else
    {
        p->head = worker;
    }
    
    p->queue_size++;
    pthread_mutex_unlock(&(p->queue_lock));
    pthread_cond_signal(&(p->queue_ready));

    return 0;
}

void 
thread_pool_wait(thread_pool_t *pool)
{
    thread_info_t *thread = NULL;
    int i = 0;

    assert(pool);

    for(i = 0; i < pool->num; i++)
    {
        thread = (thread_info_t*)(pool->threads + i);
        thread->state = THREAD_STATE_EXIT;
        pthread_join(thread->id, NULL);
    }
}

void 
thread_pool_destory(thread_pool_t *pool)
{
    thread_pool_t *p = NULL;
    thread_worker_t *member = NULL;
    thread_info_t *tmp=NULL;

    assert(pool);

    p = pool;
    if(p->is_destroy)
    {
        return ;
    }
    
    p->is_destroy = true;
    pthread_cond_broadcast(&(p->queue_ready));
    
    thread_pool_wait(pool);

    while(p->head)
    {
        member = p->head;
        p->head = member->next;
        free(member);
    }
    
    while(p->threads)
    {
        tmp = p->threads;
        p->threads = tmp->next;
        free(tmp);
    }

    free(p);
    p = NULL;
    
    pthread_mutex_destroy(&(p->queue_lock));
    pthread_cond_destroy(&(p->queue_ready));
    
    return ;
}

thread_info_t *
get_thread_by_id(thread_pool_t *pool,pthread_t id)
{
    thread_info_t *thread = NULL;

    assert(pool);
    
    thread = pool->threads;
    while(thread != NULL)
    {
        if(thread->id == id)
        {
            return thread;
        }
        
        thread = thread->next;
    }
    
    return NULL;
}

static void *
thread_excute_route(void *arg)
{
    thread_worker_t *worker = NULL;
    thread_info_t *thread = NULL; 
    thread_pool_t *p = NULL;
    pthread_t pthread_id = 0;

    assert(arg);
    
    p = (thread_pool_t*)arg;

    //printf("thread %ld create success\n", pthread_self());
    while(1)
    {
        pthread_mutex_lock(&(p->queue_lock));

        pthread_id = pthread_self();
        thread = get_thread_by_id(p, pthread_id);
        
        if(p->is_destroy == true && p->queue_size == 0)
        {
            pthread_mutex_unlock(&(p->queue_lock));

            thread->state = THREAD_STATE_EXIT;
            p->knum++;
            p->rnum--;

            pthread_exit(NULL);
        }
        
        if(thread)
        {
            thread->state = THREAD_STATE_TASK_WAITING;
        }
        
        while(p->queue_size == 0 && !p->is_destroy)
        {
            pthread_cond_wait(&(p->queue_ready),&(p->queue_lock));
        }
        
        p->queue_size--;
        worker = p->head;
        p->head = worker->next;
        pthread_mutex_unlock(&(p->queue_lock));
        
        if(thread)
        {
            thread->state = THREAD_STATE_TASK_PROCESSING;
        }
        
        (*(worker->process))(worker->arg);

        if(thread)
        {
            thread->state = THREAD_STATE_TASK_FINISHED;
        }
        
        free(worker);
        worker = NULL;
    }
}

static void *
thread_pool_is_need_extend(void *arg)
{
    thread_pool_t *p = NULL;
    thread_pool_t *pool = NULL;
    int incr = 0;
    int i = 0;
    thread_info_t *tmp=NULL;
    
    assert(arg);
    
     p =  (thread_pool_t *)arg;
     pool = p;
    
    if(p->queue_size > 100)
    {
        if(((float)p->rnum / p->queue_size) < THREAD_BUSY_PERCENT )
        {
            incr = (p->queue_size * THREAD_BUSY_PERCENT) - p->rnum;
            
            pthread_mutex_lock(&pool->queue_lock);
            
            if(p->queue_size < 100)
            {
                pthread_mutex_unlock(&pool->queue_lock);
                return ;
            }
            
            for(i = 0; i < incr; i++)
            {
                tmp = (thread_info_t*)malloc(sizeof(thread_info_t));
                if(tmp == NULL)
                {
                    continue;
                }
                else
                {
                    tmp->next = p->threads;
                    p->threads = tmp;
                }
                
                p->num++;
                p->rnum++;
                
                pthread_create(&(tmp->id), NULL, thread_excute_route, p);
                tmp->state = THREAD_STATE_RUN;
            }
            
            pthread_mutex_unlock(&pool->queue_lock);
        }
    }
    
    //pthread_cond_signal(&pool->extend_ready);

    return ;
}

static void *
thread_pool_is_need_recovery(void *arg)
{
    thread_pool_t *pool = NULL;
    int i = 0;
    thread_info_t *tmp = NULL, *prev = NULL, *p1 = NULL;

    assert(arg);

    pool = (thread_pool_t *)arg;
    
    while(1)
    {
        i=0;
        if(pool->queue_size == 0 && pool->rnum > pool->init_num )
        {
            sleep(5);
            
            if(pool->queue_size == 0 && pool->rnum > pool->init_num )
            {
                pthread_mutex_lock(&pool->queue_lock);
                
                tmp = pool->threads;
                while((pool->rnum != pool->init_num) && tmp)
                {
                    if(tmp->state != THREAD_STATE_TASK_PROCESSING)
                    {
                        i++;
                        
                        if(prev)
                        {
                            prev->next    = tmp->next;
                        }
                        else
                        {
                            pool->threads =  tmp->next;
                        }
                        
                        pool->rnum--;
                        pool->knum++;
                        
                        kill(tmp->id, SIGKILL);
                        
                        p1  = tmp;
                        tmp = tmp->next;
                        free(p1);
                        continue;
                    }
                    
                    prev = tmp;
                    tmp  = tmp->next;
                }
                
                pthread_mutex_unlock(&pool->queue_lock);
            }
        }
        
        sleep(5);
    }

    return ;
}

static void *
display_thread(void *arg)
{
    thread_pool_t *p = NULL;
    thread_info_t *thread = NULL;

    assert(arg);

    p = (thread_pool_t *)arg;
    
    while(1)
    {
        printf("threads %d,running %d,killed %d\n", p->num, p->rnum, p->knum);
        
        thread = p->threads;
        while(thread)
        {
            printf("id = %ld, state = %s\n", thread->id, thread_state_map[thread->state]);
            thread = thread->next;
        }
        
        sleep(5);
    }
}
