#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/time.h>


#ifndef SpiderMain
#define SpiderMain       main
#endif

#ifndef SPIDER_NULL_FILE
#define SPIDER_NULL_FILE "/dev/null"
#endif

#ifndef SPIDER_LOCK_FILE
#define SPIDER_LOCK_FILE "/var/run/spider.pid"
#endif

#ifndef SPIDER_SOCKET_FILE
#define SPIDER_SOCKET_FILE "/tmp/spider.sock"
#endif

#ifndef SPIDER_BOOL
#define SPIDER_BOOL
#define bool int
#define true  1
#define false 0
#endif

#ifndef SPIDER_THREAD_NUM
#define SPIDER_THREAD_NUM  (100)
#endif

#ifndef SPIDER_PROTOCOL_MARK
#define SPIDER_PROTOCOL_MARK    (0x73706472)        /* spdr */
#endif

#ifndef SPIDER_PROTOCOL_MAXLEN
#define SPIDER_PROTOCOL_MAXLEN  (512)
#endif
