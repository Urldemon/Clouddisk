#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fcgi_config.h"
#include "fcgi_stdio.h"

int main(){
    // 阻塞等待用户链接
    while(FCGI_Accept() >= 0)
    {
        char *contentLen = getenv("CONTENT_LENGTH");
        int len ;
        printf("Content_type: text/html\r\n\r\n");
        if(contentLen == NULL)
        {
            len = 0;
        }else{
            len = atoi(contentLen);
        }

        if(len <= 0)
        {
            printf("NO data from standard input <p>\n");

        }else{
            char buf[4096] = {0};
            int ret = 0;
            char *out = NULL;
            ret = fread(buf,1,4096,stdin);      // 从标准输入库中读取
            if(ret == 0){
                continue;
            }
        }

    }
}
