#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "sock_init.h"

#define BACKLOG 10

int sock_init(char *ip,int port)
{
    struct      sockaddr_in serv_addr;
    int         sock_fd = -1;

    if((sock_fd=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        printf("create TCP socket failure:%s\n",strerror(errno));
        return -1;
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    if(!ip)
        serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    else
        inet_aton(ip,&serv_addr.sin_addr);
    if(bind(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
    {
        printf("create a socket failure");
        return -1;
    }

    if(listen(sock_fd,BACKLOG)<0)
        return -2; 
    return sock_fd;

}
