/*********************************************************************************
 *      Copyright:  (C) 2019 LingYun<lingyun@email.com>
 *                  All rights reserved.
 *
 *       Filename:  tlv_client.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(24/05/19)
 *         Author:  LingYun <lingyun@email.com>
 *      ChangeLog:  1, Release initial version on "24/05/19 21:19:36"
 *                 
 ********************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "sock_init.h"
#include "get_temper.h"
#include "crc-itu-t.h"
#define THE_HEAD    0xFD
#define MIN_SIZE    5
#define delims      "|"


int go_stop = 0;

void sig_handler(int sig_t)
{
    if(SIGTERM==sig_t)
    go_stop = 1;
}

void printf_usage(char *progname)
{
    printf("%s usage:\n",progname);
    printf("-i[IP]:server IP address.\n");
    printf("-p[port]:port\n");

    printf("-t[sleeptime]:sleeptime\n");
	printf("Example:%s -i 127.0.0.1 -p 9990 -t 30\n",progname);
	return ;
}



int main(int argc , char *argv[])
{   
    char                            buf1[100] ;
   	char                            pack_buf[64];
    char                            temper_buf[64];
   	char                            id_buf[16];
    int                             p     =   0;
    char                            * progname;
    int                             sockfd          =   -1;
    
    int                             opt;         
    int                             port        =   0;
    int                             sleeptime	  =   0;
    
	int                             datalen     =   0;
    int                             pack_len    =   0;
    int                             crc16       =   0;
    int                             byte        =   0;
    char                            *command;
    char                            **pptr;
	char                            *servip;
    int                             servport;
    char 				            *ipv4;
    struct 	hostent                 *ipaddr;

    signal(SIGKILL,sig_handler);

    struct option opts[]={
        {"ip" ,required_argument  , NULL,'i'},
        {"port" ,required_argument  , NULL,'p'},
        {"sleeptime" ,required_argument  , NULL,'t'},
		{NULL,0,NULL,0}
    };

    while((opt = getopt_long(argc, argv, "i:p:t:", opts, NULL)) != -1)
    {        
        switch(opt)
        {
            case 'i':
                servip = optarg;
                break;
            case 'p':
                servport = atoi(optarg);
                break;
            case 't':
                sleeptime = atoi(optarg);
                break;
        }
    }
 	
    
    if(!servip || !servport || !sleeptime)
    {
        printf_usage(progname);
        return 0;
    }
	
    
    printf("请输入ID:");
	scanf("%s",&id_buf);
	
    
    while(!go_stop)
	{	
		if(sockfd<0)
		{
			if((sockfd = sock_init(servip,servport)) < 0)
			{
				printf("Sock_init() failure\n");
				sleep(1);
                
			}
			else
            {
                printf("Connect to server ok\n");
            }
			
		}
		if(sockfd>=0)
		{
            
            memset(pack_buf,0,sizeof(pack_buf));

        //id part;
            pack_buf[p] = (unsigned short )THE_HEAD;
            p+=1;

            pack_buf[p] = 0xFE;
            p+=1;
            datalen = strlen(id_buf)+5+2+1;
            pack_buf[p] = datalen;
            p+=1;

            datalen = strlen(id_buf);
           	memcpy(pack_buf+p,id_buf,datalen);
            p+=datalen ;

        	pack_buf[p] = '|';
        	p+=1;

        //temper;
        	float temper=get_temper();
        	int temper1 = (int)temper;
            if(temper1>temper)
                temper1-=1;
        	int temper2=(int)((temper-temper1)*100);
        	
            pack_buf[p] = temper1;
        	p+=1;
        	pack_buf[p] = temper2;
            p+=1;
            
            crc16 = crc_itu_t(IoT_MAGIC_CRC,pack_buf,p);
            ushort_to_bytes(&pack_buf[p],crc16);
            pack_buf[2] = p+2;
            pack_len = p+2;
 
		    printf("start to write to server\n");
            if(write(sockfd,pack_buf,pack_len) < 0)
            {
                printf("Write data to server failure:%s\n",strerror(errno));
                close(sockfd);
                sockfd = -1;
            }
            else
            {
                printf("Write data to server ok\n");
                p=0;
            }
            int rv;
            if((rv=read(sockfd,buf1,sizeof(buf1))) <= 0)
            {
                printf("read from server failure or get disconnected\n");
                sleep(1);
                close(sockfd);
                sockfd=-1;
           }
            else
            {
                printf("read %d  data from server \n",rv);

            }
            
		}
    printf("start sleeping\n");	
	sleep(sleeptime);
	}//while

}//main
  	



