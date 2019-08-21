#include <stdio.h>
#define delims "|"

#include <string.h>
#include "byte_to_str.h"
void byte_to_str(char *true_buf,char result[][32],char *time_buf)
{       
    strcpy(result[0],strtok(true_buf,delims));
    //strcpy(result[1],strtok(NULL,delims));
    snprintf(result[1],sizeof(result[1]),"%d-%d-%d %d:%d:%d",result[1][0]+1900,result[1][1],result[1][2],result[1][3],result[1][4],result[1][5]);

    strcpy(result[1],strtok(NULL,delims));
    snprintf(result[1],sizeof(result[1]),"%d.%dC",result[1][0],result[1][1]);
    strcpy(result[2],time_buf);
    
     
    printf("final data :%s:%s\n",result[0],result[1]);
    printf("time:%s\n",result[2]);
    
}

