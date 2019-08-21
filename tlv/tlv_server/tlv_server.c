#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sqlite3.h>
#include"get_msg.h"
#include"crc-itu-t.h"
#include"byte_to_str.h"
#include"sock_init.h"

#define MAX_EVENTS 512
#define ARRAY_SIZE(x) (size(x)/size(x[0]))
#define SIZE 64
#define MESSAGE "get data"



void set_sock_rlimit(void);
int  rw_to_sqlite(char result[][32]);
char get_time(char *datime);



void print_usage(char *progname)
{
    printf("usage:%s ....\n",progname);
    printf("-b[daemon]:set program runing to background\n");
    printf("-p[port]:set server port\n");
    printf("-h[help]:print help imformation\n");
    printf("Example:%s -b -p 9990 \n",progname);
}




int main(int argc,char * argv[])
{

    int                 opt;
    int                 servport = 0;
    char                  *progname = NULL;
    int                 listenfd,connfd,epollfd;
    int                 events;
    int                 rv;
    int                 daemon_run = 0;

    struct epoll_event  event;
    struct epoll_event  event_array[MAX_EVENTS];
    int                 a=0;
    int                *flag=&a;
    
	char 				buf[SIZE];
    char 				true_buf[SIZE];
    char 				result[2][32];
   	char 				datime[32];
    int                 byte; 
    
    
    
    struct option  opts[]=
    {
        {"daemon",no_argument,NULL,'b'},
        {"port",required_argument,NULL,'p'},
        {"help",no_argument,NULL,'h'},
        {NULL,0,NULL,0}
    };
    
    progname = basename(argv[0]);
    
    while((opt = getopt_long(argc,argv,"bp:h",opts,NULL)) != -1)
    {
        switch(opt)
        {
            case'b':
                daemon_run = 1;
                break;
            case'p':
                servport = atoi(optarg);
                break;
            case'h':
                print_usage(progname);
                break;
            default:
                break;
        }
    
    }

    if(!servport)
    {
        print_usage(progname);
        return -1;
    }

    set_sock_rlimit();

    if((listenfd = sock_init(NULL,servport)) < 0 )
    {
        printf("%s server listen on port [%d] failure:%s\n",argv[0],servport,strerror(errno));
        return -2;
    }
    printf("%s server strat listen on port[%d]\n",argv[0],servport);

    /* 

    if(daemon_run)
    {
        daemon(0,0)

    }
    
    */

    if( ( epollfd = epoll_create(MAX_EVENTS)) < 0)
    {
        printf("epoll_create() error:%s \n",strerror(errno));
        return -3;
    }

    event.events = EPOLLIN;
    event.data.fd = listenfd;

    if(epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&event) < 0)
    {
        printf("epoll_ctl() error:%s \n",strerror(errno));
        return -4;
    }



    for( ; ; )
    {
        events = epoll_wait(epollfd,event_array,MAX_EVENTS,-1);
        
        if(events < 0)
        {
            printf("epoll failure:%s\n",strerror(errno));
            break;
        }
        else if(events == 0)
        {
            printf("time out\n");
            continue;
        }
        
        
        for(int i=0;i<events;i++)
        {
            if((event_array[i].events&EPOLLERR < 0) || (event_array[i].events&EPOLLHUP))
            {
                printf("epoll_wait() get error on fd[%d] \n",event_array[i].data.fd);
                epoll_ctl(epollfd,EPOLL_CTL_DEL,event_array[i].data.fd,NULL);
                close(event_array[i].data.fd);        
            }
            
			//listen socket get events means new client start connect now
            
            if(event_array[i].data.fd == listenfd)
            {
                if((connfd = accept(listenfd,(struct sockaddr *)NULL,NULL)) < 0)
                {
                    printf("Accept new client failure:%s\n",strerror(errno));
                    continue;
                }
                
                event.data.fd = connfd;
                event.events = EPOLLIN;

                if(epoll_ctl(epollfd,EPOLL_CTL_ADD,connfd,&event) < 0)
                {
                    printf("Use epoll_ctl() to add new clientfd failure:%s\n",strerror(errno));
                    close(event_array[i].data.fd);
                    continue;
                }
                printf("Add new successfully\n");
            }


            else//already connected client data coming;
            {
                if((rv = read(event_array[i].data.fd,buf,sizeof(buf))) <= 0 )
                {
                    printf("Read data failure or get disconnected :%s\n",strerror(errno));
                    epoll_ctl(epollfd,EPOLL_CTL_DEL,event_array[i].data.fd,NULL);
                    close(event_array[i].data.fd);
                    continue;
                }
                
                else
                {
                    printf("Socket [%d] read and get %d bytes data \n",event_array[i].data.fd,rv);
                    

                    byte = get_msg(buf,true_buf);

                    if(byte > 0)
                    {
                        get_time(datime);

                        byte_to_str(true_buf,result,datime);
                    }
                    else
                    {
                        printf("Get false message:%s\n",strerror(errno));
                        break;
                    }
                    
                    if(rw_to_sqlite(result) < 0)
                    {
                        printf("Write data to sqlite faiure:%s\n",strerror(errno));
                    }
                    
					if(write(event_array[i].data.fd,MESSAGE,strlen(MESSAGE)) < 0)
                    {
                        printf("socket %d write failure:%s \n",event_array[i].data.fd,strerror(errno));
                        epoll_ctl(epollfd,EPOLL_CTL_DEL,event_array[i].data.fd,NULL);
                        close(event_array[i].data.fd);
                    }
                    
                }
            }
            
        }
    }//for(;;)

CleanUp:
    close(listenfd);
    return 0;

}//main




void set_sock_rlimit(void)
{
    struct rlimit limit = {0};
    getrlimit(RLIMIT_NOFILE,&limit);
    limit.rlim_cur = limit.rlim_max;
    setrlimit(RLIMIT_NOFILE,&limit);

    printf("set socket open fd max count  to %d \n",limit.rlim_max);
}



int rw_to_sqlite(char result[][32])

{
    
    char            *perrMsg,*rt;
    int                 ret;
    sqlite3         *db = 0;
    
    
    
    if( access("temper.db",0) != 0)
    {
        ret = sqlite3_open("temper.db",&db);
        
        if(ret != SQLITE_OK)
        {
            printf("open sqlite failure:%s\n",strerror(errno));
            return -1;
        }
        printf("open sqlite ok\n");

        
        
        const char *sql = "create table TEMPER(cid integer primary key autoincrement,ID varchar(20),TIME varchar(30),TEMPER varchar(10));";
        
        ret = sqlite3_exec(db,sql,0,0,&perrMsg);
        if(ret != SQLITE_OK)
        {
            printf("create table failure\n");
            sqlite3_free(perrMsg);
            return -2;
        }

        printf("create table ok\n");    
    
        
        
        rt = sqlite3_mprintf("insert into TEMPER VALUES(NULL,'%s','%s','%s')",result[0],result[2],result[1]);
        
        ret = sqlite3_exec(db,rt,0,0,&perrMsg);
        if(ret != SQLITE_OK)
        {
            printf("write to sqlite failure:%s\n",strerror(errno));
            return -2;
        }
        
        printf("write data to sqlite ok\n");

        sqlite3_free(perrMsg);
        sqlite3_close(db); 
        db =0;
        printf("close sqlite ok\n");
        return 0;

    }
    else
    {
       	ret = sqlite3_open("temper.db",&db);
    	
        
        if(ret != SQLITE_OK)
    	{
        	printf("open sqlite failure\n");
        	return -1;
    	}
    	printf("open sqlite ok\n");
		
		
        rt = sqlite3_mprintf("insert into TEMPER VALUES(NULL,'%s','%s','%s')",result[0],result[2],result[1]);
    	
        ret = sqlite3_exec(db,rt,0,0,&perrMsg);
    	if(ret != SQLITE_OK)
    	{
        	printf("write to sqlite failure:%s\n",strerror(errno));
        	return -2;
    	}
    	printf("write data to sqlite ok\n");

    	sqlite3_free(perrMsg);
    	sqlite3_close(db);
    
    	db =0;
    	printf("close sqlite ok\n");
    	return 0;	
 
    }
}//rw_to_sqlite;



char get_time(char *datime)
{
    struct  tm *pt;
    int     cur_sec, cur_min, cur_hour, cur_day, cur_mouth, cur_year;
    time_t  t ;
    
    
    time(&t);
    pt = localtime(&t);

    cur_sec = pt->tm_sec;
    cur_min = pt->tm_min;
    cur_hour = pt->tm_hour;
    cur_day = pt->tm_mday;
    cur_mouth = pt->tm_mon + 1;
    cur_year = pt->tm_year + 1900;
        
    snprintf(datime,19,"%d-%d-%d %d:%d:%d ", cur_year, cur_mouth, cur_day,cur_hour,cur_min,cur_sec);
    return 0;

}//get_sys_time;
