
#include "common.h"

spider_option_t options = {-1, 0, -1, NULL}; 

int 
create_deamon()
{
    pid_t pid = 0;
    int nullfd = -1;
    int ret = 0;
    
    /* create a deamon proccess */
    pid = fork();
    if(pid < 0)
    {
        printf("fork error, please check your computor.\n");
        exit(127);
    }
    else if(pid > 0) //this proccess is parent
    {
        exit(0);//bye-bye
    }

    nullfd = open(SPIDER_NULL_FILE, O_WRONLY, 0);
    if(nullfd == -1)
    {
        printf("open %s error nullfd = %d\n", SPIDER_NULL_FILE, nullfd);
        perror("open failed");

        return -1;
    }

    dup2(nullfd, 0);
    dup2(nullfd, 1);
    dup2(nullfd, 2);

    close(nullfd);

    ret = setsid();
    if(ret == -1)
    {
        printf("setsid error\n");
        return -1;
    }

    umask(0);

    ret = chdir("/");
    if(ret == -1)
    {
        printf("chdir error\n");
        return -1;
    }

    options.pid = getpid();

    return 0;
}

int
create_lock_file()
{
    int ret = 0;
    struct flock fl;
    int fd = -1;
    char lock[16] = {0};
    
    /* check proccess wether in system */
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    fl.l_pid = getpid();

    fd = open(SPIDER_LOCK_FILE, O_CREAT|O_RDWR, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
    if(fd < 0)
    {
        printf("open %s error.\n", SPIDER_LOCK_FILE);
        perror("open failed");

        return -1;
    }

    ret = fcntl(fd, F_SETLK, &fl);
    if(ret == -1)
    {
        if(errno == EACCES || errno == EAGAIN)
        {
            printf("this proccess already run in system.\n");
            close(fd);
            return -1;
        }
    }

    ret = ftruncate(fd, 0);
    if(ret == -1)
    {
        printf("ftruncate error.\n");
        perror("ftruncate error");

        return -1;
    }

    snprintf(lock, 16, "%ld", (long)getpid());

    ret = write(fd, lock, strlen(lock) + 1);
    if(ret != (strlen(lock)+1))
    {
        printf("write lock file error. ret = %d\n", ret);
        perror("write failed");

        return -1;
    }

    options.lckfd = fd;

    return 0;
}

void *
local_socket_callback(void * args)
{
    spider_option_t *op = NULL;
    int clisockfd = -1;
    int svrsockfd = -1;
    socklen_t len = 0;
    struct sockaddr_un cliaddr; 
    int ret = 0;
    spider_protocol_head_t ph;
    char *data = NULL;
    
    if(args == NULL)
    {
        printf("callback parameter is null.\n");
        return NULL;
    }
    
    op = (spider_option_t *)args;
    svrsockfd = op->lsock;

    len = sizeof(cliaddr);

    data = malloc(SPIDER_PROTOCOL_MAXLEN);
    if(data == NULL)
    {
        printf("malloc error.\n");

        return NULL;
    }
    
    while(1)
    { 
        clisockfd = accept(svrsockfd, (struct sockaddr *)&cliaddr, &len);
        if(clisockfd == -1)
        {
            printf("socket accept error.\n");
            perror("accept error");

            return NULL;
        }

        while(1)
        {
            bzero(&ph, sizeof(spider_protocol_head_t));
            printf("reading...\n");
            ret = read(clisockfd, &ph, sizeof(spider_protocol_head_t));
            if(ret != sizeof(spider_protocol_head_t))
            {
                printf("read client header data error.\n");
                perror("read error");

                break;
            }

            printf("*******\n");
            printf("mark = 0x%x\n", ph.mark);
            printf("type = %d\n", ph.type);
            printf("subtype = %d\n", ph.subtype);
            printf("length = %d\n", ph.length);

            ret = read(clisockfd, data, ph.length);
            if(ret != ph.length)
            {
                printf("read client data error.\n");
                perror("read error");

                break;
            }
        }
    }
}

int 
create_local_socket()
{
    int ret = 0;
    struct sockaddr_un svraddr;
    int svrsockfd = -1;
    int on = 1;
    thread_pool_t * pool = NULL;
    struct stat s;

    if(options.tpool == NULL)
    {
        printf("thread pool not exsit, please create it before\n");
        return -1;
    }

    pool = options.tpool;
    
    bzero(&svraddr, sizeof(struct sockaddr_un));
    bzero(&s, sizeof(struct stat));

    svraddr.sun_family = AF_UNIX;
    strncpy(svraddr.sun_path, SPIDER_SOCKET_FILE, sizeof(svraddr.sun_path));

    ret = stat(SPIDER_SOCKET_FILE, &s);
    if(ret == 0)
    {        
        ret = unlink(SPIDER_SOCKET_FILE);
        if(ret == -1)
        {
            printf("unlink socket file error %s.\n", SPIDER_SOCKET_FILE);
            perror("unlink error");

            return -1;
        }
    }

    svrsockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(svrsockfd == -1)
    {
        printf("socket error.\n");
        perror("socket error");

        return -1;
    }

    ret = setsockopt(svrsockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
    {
        printf("setsockopt error.\n");
        perror("setsockopt error");

        return -1;
    }

    ret = bind(svrsockfd, (struct sockaddr *)&svraddr, sizeof(svraddr));
    if(ret == -1)
    {
        printf("bind address error.\n");
        perror("bind error");

        return -1;
    }

    ret = listen(svrsockfd, 5);
    if(ret == -1)
    {
        printf("listen error.\n");
        perror("listen error");

        return -1;
    }

    options.lsock = svrsockfd;

    ret = thread_pool_add_worker(pool, local_socket_callback, &options);
    if(ret == -1)
    {
        printf("thread add one worker failed.\n");

        return -1;
    }
    
    return 0;
}

int 
create_thread_pool()
{
    thread_pool_t * pool = NULL;
    int ret = 0;
    
    pool = thread_pool_create(SPIDER_THREAD_NUM);
    if(pool == NULL)
    {
        printf("thread pool creat failed.\n");

        ret = -1;
        return ret;
    }

    options.tpool = pool;
    
    return 0;
}

int 
SpiderMain(int argc, char * argv[])
{
    int ret = 0;
#if 0
    ret = create_deamon();
    if(ret == -1)
    {
        printf("create deamon proccess error!\n");
        return -1;
    }
#endif
    ret = create_lock_file();
    if(ret == -1)
    {
        printf("create lock file error!\n");
        return -1;
    }

    ret = create_thread_pool();
    if(ret == -1)
    {
        printf("create thread pool error!\n");
        return -1;
    }

    ret = create_local_socket();
    if(ret == -1)
    {
        printf("create local socket error!\n");
        return -1;
    }

    for(;;)
    {
        printf("for   loop!\n");
        sleep(5);
    }
    
    return 0;
}
