#include "def.h"

#include <pthread.h>

typedef struct  thread_worker_s{
    void *(*process)(void *arg);
    void *arg;
    struct thread_worker_s *next;
}thread_worker_t;

#define THREAD_STATE_RUN               0
#define THREAD_STATE_TASK_WAITING      1
#define THREAD_STATE_TASK_PROCESSING   2
#define THREAD_STATE_TASK_FINISHED     3
#define THREAD_STATE_EXIT              4     


typedef struct thread_info_s{
    pthread_t               id;
    int                     state;  
    struct thread_info_s   *next;
}thread_info_t;

typedef struct thread_pool_s{
    pthread_mutex_t     queue_lock ;
    pthread_cond_t      queue_ready;

    thread_worker_t    *head;
    bool                is_destroy;
    int                 num;
    int                 rnum;
    int                 knum;
    int                 queue_size;
    thread_info_t      *threads;
    pthread_t           display;
    pthread_t           destroy;
    pthread_t           extend;
    float               percent;
    int                 init_num;
    pthread_cond_t      extend_ready;
}thread_pool_t;

thread_pool_t* thread_pool_create(int num);
int thread_pool_add_worker(thread_pool_t *pool,void*(*process)(void *arg),void *arg);
void thread_pool_destory(thread_pool_t *pool);
