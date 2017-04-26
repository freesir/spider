#include <stdio.h>

#include "common.h"

int
main(int argc, char * argv[])
{
    int sockfd = -1;
    struct sockaddr_un adress;
    adress.sun_family = AF_UNIX;
    strncpy(adress.sun_path, "/tmp/spider.sock", sizeof(adress.sun_path));
    int ret = 0;
    spider_protocol_head_t ph;
    char buf[7] = {0};
    int i = 0;
    
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        printf("creat sockfd error.\n");
        perror("creat sockfd");

        return -1;
    }

    ret = connect(sockfd, (struct sockaddr *)&adress, sizeof(adress));
    if(ret == -1)
    {
        printf("connect sock error.\n");
        perror("connect error");

        return -1;
    }

    for(i = 0; i < 100; i++)
    {
        sleep(1);
        ph.mark = SPIDER_PROTOCOL_MARK;
        ph.length = 6;
        ph.type = 1;
        ph.subtype = 2;

        ret = write(sockfd, &ph, sizeof(ph));
        if(ret != sizeof(ph))
        {
            printf("write protocol header error.\n");
            perror("write error");

            return -1;
        }

        strncpy(buf, "spider", sizeof(buf));

        ret = write(sockfd, buf, 6);
        if(ret != 6)
        {
            printf("write protocol data error.\n");
            perror("write error");

            return -1;
        }

        printf("write one protocol data.\n");
    }

    close(sockfd);
    sockfd = -1;

    return 0;
}
