#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include "sock_init.h"

int sock_init( char *serv_ip, int serv_port)
{
    int sockfd = -1;
    struct sockaddr_in serv_addr;
    if((sockfd=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        printf("Use socket to creat TCP socket failure:%s\n",strerror(errno));                                                 
        return -1;
    }
    else if(sockfd > 0)
    {
        memset(&serv_addr,0,sizeof(serv_addr));
        serv_addr.sin_family = AF_INET; 
        serv_addr.sin_port = htons(serv_port);
        inet_aton(serv_ip,&serv_addr.sin_addr);
        if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        {   
            printf("connect server failure:%s\n",strerror(errno));
            return -2;
        }
        else
            return sockfd;
    }
}
