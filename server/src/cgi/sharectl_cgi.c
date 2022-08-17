#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "define.h"
#include "cfg.h"
#include "mysql_c_aip.h"
#include "redis_c_api.h"
#include "redis_keys.h"
#include "cJSON.h"

int get_json_data(char *user_name,char *token,char *md5,char *file_name);
int pv_file(const char *md5,const char *file_name);
int save_file(const char *user_name,const char *md5,const char *file_name);
int main()
{
    while (FCGI_Accept() >= 0) 
    {
        int ret = 0;
        char *query = NULL;
        char cmd[256] = {0};
        char user_name[USER_LEN] = {0};
        char token[TOKEN_LEN] = {0};
        char md5[MD5_LEN] = {0};
        char file_name[FILE_NAME_LEN] = {0};

        printf("Content-type: text/html\r\n\r\n");

        char *conlength = getenv("CONTENT_LENGTH");
        if(conlength != NULL && atoi(conlength) > 0)
        {
            query = getenv("QUERY_STRING");
            // 获取url上的参数
            ret = get_query_data(query,"cmd",cmd);
            if(ret == -1)goto END;

            // 获取json上的数据
            ret = get_json_data(user_name,token,md5,file_name);
            if(ret == -1)goto END;

            // 验证登录
            ret = verify_token(user_name,token);
            if(ret != 0)goto END;

            // 执行命令
            if(strcmp(cmd,"pv") == 0)
               ret = pv_file(md5,file_name);
            else if(strcmp(cmd,"save") == 0)
               ret = save_file(user_name,md5,file_name);
            else ret = -1;

        }else ret = -1;
END: 
        if(ret == -1)printf(respost_code("114",NULL,NULL));         // 出错
        if(ret == 0)printf(respost_code("110",NULL,NULL));          // 操作成功
        if(ret == 1)printf(respost_code("111",NULL,NULL));          // 账户验证失败
        if(ret == 2)printf(respost_code("112",NULL,NULL));          // 文件已经存在
            
    }
    return 0;
}


int get_json_data(char *user_name,char *token,char *md5,char *file_name)
{
    int ret = 0;
    char buf[4096] = {0};
    cJSON *root = NULL;

    ret = fread(buf,1,4096,stdin);
    if(ret <= 0)
    {
        ret = -1;
        goto END;
    }

    root = cJSON_Parse(buf);
    if(root == NULL)
    {
        ret = -1;
        goto END;
    }

    cJSON *obj = cJSON_GetObjectItem(root,"user_name");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(user_name,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"token");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(token,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"md5");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(md5,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"file_name");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(file_name,obj->valuestring);

END:
    if(root != NULL)cJSON_Delete(root);
    return ret;
}

int pv_file(const char *md5,const char *file_name)
{
    int ret = 0;
    char key[1024] = {0};
    int pv = 0;
    MYSQL *my = NULL;
    redisContext *rd = NULL;
    char command[SQL_COM_LEN] = {0};
    char tmp[256] = {0};

    // 链接mysql 和 redis
    my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }
    rd = redis_conn_init();
    if(rd == NULL)
    {
        ret = -1;
        goto END;
    }

    sprintf(key,"%s%s",md5,file_name);
    
    // 先去redis上看是否能获取到数据  -1 不存在  存在返回socre数
    if((pv = redis_zscore(rd,FILE_PUBLIC_ZSET,key)) == -1)
    {
        sprintf(command,"select pv from share_file_list where md5='%s' and file_name='%s'",md5,file_name); 
        if(mysql_result_one(my,command,strlen(command),tmp) != 0)
        {
            ret = -1;
            goto END;
        }
        pv = atoi(tmp);
    }

    // 修改mysql上的数据
    memset(command,0,sizeof(command));
    sprintf(command,"update share_file_list set pv=%d where md5='%s' and file_name='%s'",pv+1,md5,file_name);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret = -1;
        goto END;
    }

    // 修改redis上的数据
    redis_zadd(rd,FILE_PUBLIC_ZSET,pv+1,key);
END:
    if(my != NULL)mysql_conn_close(my);
    if(rd != NULL)redis_conn_free(rd);
    return ret;
}

int save_file(const char *user_name,const char *md5,const char *file_name)
{
    int ret = 0;
    int ret2 = 0;
    char tmp[256] = {0};
    MYSQL *my = NULL;
    redisContext *rd = NULL;
    char command[SQL_COM_LEN] = {0};
    char create_time[TIME_LEN] = {0};

    time_t now;
    now = time(NULL);
    strftime(create_time,TIME_LEN-1,"%Y-%m-%d %H:%M:%S",localtime(&now));
    
    // 链接mysql和redis
    my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }

    // ======启动事务
    if(mysql_conn_query(my,"begin",strlen("begin"),NULL) != 0)
    {
        ret = -1;
        goto END;
    }

    // 查询用户下是否有该文件
    sprintf(command,"select * from user_file_list where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
    ret = mysql_result_one(my,command,strlen(command),NULL);// 1表示没有记录 0表示存在 -1 出错
    if(ret != 1)
    {
        ret = ret == 0 ? 2:-1;
        goto END;
    }

    // 将数据加入用户列表中
    memset(command,0,sizeof(command));
    sprintf(command,"insert into user_file_list (user,md5,create_time,file_name,shared_status,pv)values ('%s','%s','%s','%s',%d,%d)",user_name,md5,create_time,file_name,0,0);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }

    // 修改用户的文件数量
    bzero(command,sizeof(command));
    sprintf(command,"select count from user_file_count where user='%s'",user_name);
    ret2 = mysql_result_one(my,command,strlen(command),tmp);
    
    bzero(command,sizeof(command));
    if(ret2 == 0)   // 列表中存在用户的count列表
        sprintf(command,"update user_file_count set count=%d where user='%s'",atoi(tmp)+1,user_name);
    else if(ret2 == 1)       // 没有用户的count列表
        sprintf(command,"insert into user_file_count (user,count) values ('%s',%d)",user_name,1);
    else  
    {
        ret2 = -1;
        goto END;
    }

    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }

    // 修改文件列表上的共享参数
    bzero(command,sizeof(bzero));
    sprintf(command,"select count from file_info where md5='%s'",md5);
    if(mysql_result_one(my,command,strlen(command),tmp) != 0)
    {
        ret2 = -1;
        goto END;
    }
    bzero(command,sizeof(bzero));
    sprintf(command,"update file_info set count=%d where md5='%s'",atoi(tmp)+1,md5);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }
    ret2 = 0;
    ret = 0;
END:
    if(ret2 == 0)mysql_conn_query(my,"commit",strlen("commit"),NULL);
    else
    {
        ret = -1;
        mysql_conn_query(my,"rollback",strlen("rollback"),NULL);
    }

    if(my != NULL)mysql_conn_close(my);
    return ret;
}

// gcc -o sharectl_cgi sharectl_cgi.c -I ../../include/ ../cfg.c ../cJSON.c ../redis_c_api.c ../mysql_c_aip.c -lmysqlclient -lhiredis -lfcgi 
