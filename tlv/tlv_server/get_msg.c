#include <stdio.h>
#include <string.h>
#include "crc-itu-t.h"
#include "get_msg.h"

#define SIZE 64

int get_msg(char buf[],char true_buf[])
{
    int len         = 0;
    int byte        = 0;
    int crc         = 0;
    int crc_ago     = 0;
    int i = 0;
    
    len = strlen(buf);

    for(i=0;i<len;i++)
    {
        if((buf[i] != 253) && (buf[i+1] != 254) && (buf[i+2] != len))
        {
            printf("Get error data and will to refuse it\n");
            break;
        }
        else
        {
            crc = crc_itu_t(MAGIC_CRC, &buf[i], len-2);
            crc_ago = bytes_to_ushort(&buf[i+len-2],2);
            
            
            
            if(crc != crc_ago)
            {
                printf("Wrong crc\n");
                continue;
            }
            else
            {
                printf("Correct data \n");
                memmove(true_buf,&buf[i+3],len-5);
                byte = len-5;
                break;

            }


        }
    }
    return byte;
}
