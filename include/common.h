#include "def.h"
#include "threadpool.h"

struct spider_option {
    int                 lckfd;          /* lock file fd */
    pid_t               pid;            /* proccess pid */
    int                 lsock;          /* local socket fd */

    thread_pool_t      *tpool;          /* thread pool addr */
};

typedef struct spider_option spider_option_t;

struct spider_protocol_head_s {
    int     mark;           /* mark string default "spdr" */
    int     type;           /* command type */
    int     subtype;        /* command sub type */
    int     length;         /* data length */
    char    data[0];        /* data header */
};

typedef struct spider_protocol_head_s spider_protocol_head_t;