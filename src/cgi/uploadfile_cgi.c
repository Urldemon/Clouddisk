#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "fcgi_stdio.h"
#include "../../include/fdfs_c_api.h"


// #include "fdfs_c_api.h"

#define TEM_BUF_MAX_LEN 1024
#define TEMP_BUF_MAX_LEN 1024
#define FDFS_CLIENT_CONF_PATH "../../conf/client.conf"
// 读取客户端文件信息

int recv_save_file(char* user,char* filename,char* md5,long* p_size)
{
    /* 变量初始化 */
    int ret = 0;
    char *file_buf = NULL;                      // 文件信息
    char *begin = NULL;                         // 文件信息位置定位指正
    char *p,*q,*k; 
    char boundary[TEM_BUF_MAX_LEN] = {0};       // 分界线信息
    char content_text[TEMP_BUF_MAX_LEN] = {0};  // 文件头部信息

    file_buf = (char *)malloc(4096);         //开辟存储数据的内存空间
    if(file_buf == NULL){
        perror("malloc error");
        return -1;
    }

    /* 读取数据 */
    int len = fread(file_buf,1,4096,stdin);          // fcgi重定向了标准输入，从这里那客户端发来的数据
    if(len == 0){
        ret = -1;
        goto END;
    }
    file_buf[len] = '\0';

    /* 开始解析数据 */
    begin = file_buf;
    p = begin;

    // 1. 获取分界线信息
    p = strstr(file_buf,"\\r\\n");            // 找到第一个出现/r/n的位置
    if(p == NULL){
        ret = -1;
        goto END;
    }
    strncpy(boundary,begin,p-begin);        // 将begin到p的数据复制到boundary上
    boundary[p-begin] = '\0';               // 字符串需要加哨兵
    p += 4;                                 // 跳过/r/n 指针p就到了第二行
    len -= p-begin;                         // 减去读取的了一部分数据
    begin = p;                              // 头哨兵指向未读数据头
    printf("boundary:%s\n",boundary);

    // 2. 获取文件头部信息
    p = strstr(begin,"\\r\\n");
    if(p == NULL){
        ret = -1;
        goto END;
    }
    strncpy(content_text,begin,p-begin);
    content_text[p-begin] = '\0';
    p += 4;
    len -= p-begin;
    printf("content_text : %s\n",content_text);

    // 3. 对获取的文件头部信息进行拆分
    // 3.1 获取用户名
    q = begin;
    q = strstr(begin,"user=");
    q += strlen("user=");
    ++q;
    k = strchr(q,'"');
    strncpy(user,q,k-q);
    user[k-q] = '\0';
    // trim_space(user);       //去掉一个字符串两边的空白

    // 3.2 获取文件名
    begin = k;
    q = begin;
    q = strstr(begin,"filename=");
    q += strlen("filename=");
    ++q;
    k = strchr(q,'"');
    strncpy(filename,q,k-q);
    filename[k-q] = '\0';

    // 3.3 获取md5数据  
    begin = k;
    q = begin;
    q = strstr(begin,"md5=");
    q += strlen("md5=");
    ++q;
    k = strchr(q,'"');
    strncpy(md5,q,k-q);
    md5[k-q] = '\0';

    // 3.5 获取数据大小
    begin = k;
    q = strstr(begin,"size=");
    q += strlen("size=");
    k = strstr(q,"\\r\\n");
    char size_str[256] = {0};
    strncpy(size_str,q,k-q);
    size_str[k-q] = '\0';
    *p_size = strtol(size_str,NULL,10);

    // 4. 获取文件编码格式和取出空格行 ,对数据传输没有影响直接跳过啊
    begin = p;
    p = strstr(p,"\\r\\n"); 
    p += 8;                           // 到达末尾有个\r\n

    len -= p - begin;       // 正文的起始位置到数组结束的长度


    // 5. 读取真正的数据
    begin = p;

    int fd = open(filename,O_CREAT | O_WRONLY,0644);
    if(fd < 0){
        ret = -1;
        goto END;
    }
    write(fd ,begin,len);
    // 1. 文件内容读完了
    // 2. 文件没有读完
    if(*p_size > len){
        // 读文件  -> 接着读post数据
        while((len = fread(file_buf,1,4096,stdin)) > 0)
        {
            //数据写入文件
            write(fd,file_buf,len);
        }   
    }
    // 上面操作结束后会把末尾的分界线都写入文件，所以需要删除

    ftruncate(fd,*p_size);                    // ftruncate会将参数fd指定的文件大小改为参数length指定的大小
    close(fd); 
END:
    free(file_buf);
    return ret;
}
int main(){
    // 开始流程
    while(FCGI_Accept() >= 0)
    {
       // 1. 读一次数据 -- buf,能够保证分割线和两个头都装进去
        char user[24] = {0};
        char filename[32] = {0};
        char md5[64] = {0};
        long size = 0;
        char file_id[256] = {0};

        printf("Content-Type: text/plain;charset=utf-8;\r\n\r\n");
        // 读取并存储数据
        recv_save_file(user,filename,md5,&size);
    
        // 将上传的文件传入fastdfs上去
        fdfs_upload_file(FDFS_CLIENT_CONF_PATH,filename,file_id);

        // 将客户端写入mysql数据库
    

        // 向客户端发送响应信息
        printf("user:%s,filename:%s,md5:%s,size:%ld -> filid = %s\n",user,filename,md5,size,file_id);
        printf("传输完成。。。。\n");


    }   
    return 0;
}

