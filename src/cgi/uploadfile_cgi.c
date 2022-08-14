#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include "fcgi_stdio.h"
#include <fcgi_config.h>
#include "cfg.h"
#include "../../include/define.h"
#include "fdfs_c_api.h"
#include "mysql_c_aip.h"

#if 1
#define GO 4
#define ENTER "\\r\\n"
#else
#define GO 2
#define ENTER "\r\n"
#endif

// 解析数据
int recv_save_file(char* user,char* filename,char* md5,long* p_size)
{
    /* 变量初始化 */
    int ret = 0;
    char *file_buf = NULL;                      // 文件信息
    char *begin = NULL;                         // 文件信息位置定位指正
    char *p,*q,*k; 
    char boundary[TEMP_BUF_MAX_LEN] = {0};       // 分界线信息
    char content_text[TEMP_BUF_MAX_LEN] = {0};  // 文件头部信息

    file_buf = (char *)malloc(4096);         //开辟存储数据的内存空间
    if(file_buf == NULL){
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
    p = strstr(file_buf,ENTER);            // 找到第一个出现/r/n的位置
    if(p == NULL){
        ret = -1;
        goto END;
    }
    strncpy(boundary,begin,p-begin);        // 将begin到p的数据复制到boundary上
    boundary[p-begin] = '\0';               // 字符串需要加哨兵
    p += GO;                                 // 跳过/r/n 指针p就到了第二行
    len -= p-begin;                         // 减去读取的了一部分数据
    begin = p;                              // 头哨兵指向未读数据头

    // 2. 获取文件头部信息
    p = strstr(begin,ENTER);
    if(p == NULL){
        ret = -1;
        goto END;
    }
    strncpy(content_text,begin,p-begin);
    content_text[p-begin] = '\0';
    p += GO;
    len -= p-begin;

    // 3. 对获取的文件头部信息进行拆分
    // 3.1 获取用户名
    q = begin;
    q = strstr(begin,"user=");
    q += strlen("user=");
    ++q;
    k = strchr(q,'"');
    strncpy(user,q,k-q);
    user[k-q] = '\0';

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
    k = strstr(q,ENTER);
    char size_str[256] = {0};
    strncpy(size_str,q,k-q);
    size_str[k-q] = '\0';
    *p_size = strtol(size_str,NULL,10);

    // 4. 获取文件编码格式和取出空格行 ,对数据传输没有影响直接跳过啊
    begin = p;
    p = strstr(p,ENTER); 
    p += 2*GO;                           // 到达末尾有个\r\n

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



// 将获取的file_id转化程服务器访问的url 存入数据库中方便访问
int set_fdfs_url(const char *file_id,char *fdfs_url)
{
    int ret = 0;
    if(file_id == NULL || fdfs_url == NULL)return -1;
    char ip[33] = {0};
    char port[17] = {0};

    // 获取当前服务器的ip地址
    if(get_conf_inet_value("server",ip,port)==-1)
    {
        ret = -1;
        goto END;
    }
    sprintf(fdfs_url,"http:/%s:%s/%s",ip,port,file_id);
END:
    return ret;
}


// 将数据写入mysql
int mysql_save_file(char *user, char *filename, char *md5,char *file_id, char *fdfs_file_url)
{
    int ret = 0;
    char type[TYPE_LEN] = {0};
    char create_time[TIME_LEN] = {0};
    char command[SQL_COM_LEN] = {0};
    
    char *begin = NULL;
    char *index = NULL;

    // 1. 初始化my对象
    MYSQL *my = mysql_conn_init();

    // 2. 写入mysql数据
    // 2.1 开启事务
    if(mysql_conn_query(my,"begin",strlen("begin"),NULL) != 0)
    {
        ret = -1;
        goto END;
    }

    // 2.2 将数据写入文件信息表中
    // 获取文件属性
    struct stat dat;
    int res = stat(filename,&dat);

    // 截取文件类型
    begin = strchr(filename,'.');
    index = strchr(filename,'\0');
    strncpy(type,begin + 1,index - begin -1);

    // 写入Mysql文件信息列表中
    memset(command,0,sizeof(command));
    sprintf(command,\
            "insert into file_info (md5,file_id,url,size,type,count)  \
            values('%s','%s','%s','%ld','%s','%d')", \
            md5,file_id,fdfs_file_url,dat.st_size,type,1);
    if(mysql_conn_query(my,command,strlen(command),NULL) == -1)
    {
        ret = -1;
        goto END;
    }

    // 2.3 将数据写入用户文件信息中
    // 写入用户文件信息表

    // 获取当前时间 并保存到create_time 下
    time_t now = time(NULL);
    strftime(create_time,TIME_LEN-1,"%Y-%m-%d %H:%M:%S",localtime(&now));

    memset(command,0,sizeof(command));
    sprintf(command,"insert into user_file_list (user,md5,create_time,file_name,shared_status,pv) \
            values ('%s','%s','%s','%s','%d','%d')",\
            user,md5,create_time,filename,0,0);
    if(mysql_conn_query(my,command,strlen(command),NULL) == -1)
    {
        ret = -1;
        goto END;
    }

    // 2.4 将文件写入fastdfs文件信息中
    // 查询文件是否有重复的
    memset(command,0,sizeof(command));
    sprintf(command,"select count from user_file_count where user = '%s'", user); 
    char tmp[512] = {0};
    int count = 0;
    int ret2 = mysql_result_one(my,command,strlen(command),tmp);

    memset(command,0,sizeof(command));
    if(ret2 == 1)           // 列表中不存在时添加数据
    {
        sprintf(command,"insert into user_file_count (user,count) values ('%s',%d)",user,1);
    }else if(ret2 == 0)     // 列表中存在时修改count值让其+1
    {
        count = atoi(tmp);
        sprintf(command,"update user_file_count set count = %d where user = '%s'",count+1,user);
    }

    if(mysql_conn_query(my,command,strlen(command),NULL) == -1)
    {
        ret = -1;
        goto END;
    }
    // 3. 提交事务
    if(mysql_conn_query(my,"commit",strlen("commit"),NULL) == -1)ret = -1;
END:
    if(ret == -1){
        // mysql回滚机制
        mysql_conn_query(my,"rollback",strlen("rollback"),NULL);
    }
    // 断开mysql链接
    mysql_conn_close(my);
    return ret;
}

    
int main(){
    // 1. 初始化数据
    char user[USER_LEN] = {0};
    char filename[FILE_NAME_LEN] = {0};
    char md5[MD5_LEN] = {0};
    long size = 0;
    char file_id[FILE_ID_LEN] = {0};
    char file_id_url[URL_LEN] = {0};

    // 开始流程
    while(FCGI_Accept() >= 0)
    {
        int ret = 0;
        // 获取请求头中de CONTENT_TYPE长度
        char *conlength= getenv("CONTENT_LENGTH"); 
        long conlen = 0;
        if(conlength != NULL)
        {
            conlen= atoi(conlength);
        }
        printf("Content-Type: text/html;charset=utf-8;\r\n\r\n");

        if(conlen > 0){
            // 读取并存储数据
            if(recv_save_file(user,filename,md5,&size) == -1)
            {
                ret = -1;
                goto END;
            }
   
            // 获取fdfs客户端配置路径
            char clinet_path[FILE_NAME_LEN] = {0};
            get_conf_value(CONF_PATH,"fastdfs","client_conf_path",clinet_path);

            // 将上传的文件传入fastdfs上去
            if(fdfs_upload_file(clinet_path,filename,file_id) != 0)
            {
                ret = -1;
                goto END;
            }

            // 将file_id转化程url格式
            set_fdfs_url(file_id,file_id_url);

            // 将客户端写入mysql数据库
            if(mysql_save_file(user,filename,md5,file_id,file_id_url) == -1)
            {
                // 删除添加到fastdfs上的文件
                fdfs_delete_file(clinet_path,file_id);
                ret = -1;
                goto END;
            }

            // 删除本地创建的文件
            unlink(filename);
        }else ret = -1;

END:
        if(ret < 0){
            printf("传输出错！\n");
        }else{
            // 向客户端发送响应信息
            printf("user:%s,filename:%s,md5:%s,size:%ld -> filid : %s\n",user,filename,md5,size,file_id_url);
            printf("传输完成。。。。\n");
        }

        // 清0数据
        bzero(user,USER_LEN);
        bzero(filename,FILE_NAME_LEN);
        bzero(md5,MD5_LEN);
        bzero(file_id,FILE_ID_LEN);
        bzero(file_id_url,URL_LEN);
        size = 0;
    }   
    return 0;
}

// gcc -o uploadfile_cgi uploadfile_cgi.c ../cJSON.c ../cfg.c ../mysql_c_aip.c  ../fdfs_c_api.c  -I ../../include/  -I /usr/include/fastdfs -lfcgi -lfdfsclient -lmysqlclient
