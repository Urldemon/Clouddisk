#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fcgi_stdio.h"
#include "fcgi_config.h"
#include "define.h"
#include "mysql_c_aip.h"
#include "cJSON.h"
#include "redis_c_api.h"
#include "cfg.h"

int get_json_data_info(char *user_name,char *token,int *start,int *count);
int push_count_data(const char *user_name);
int push_user_file_list(const char *cmd,const char *user_name,int start,int count);
void return_status(const char *retcode,int count);

int main()
{
    while(FCGI_Accept() >= 0)
    {
        int ret = 0;
        char cmd[CMD_LEN] = {0};
        char *query = NULL;
        char *conlength = NULL;

        printf("Content-type: text/html\r\n\r\n");

        conlength = getenv("CONTENT_LENGTH"); 
        if(conlength != NULL && atoi(conlength) > 0)
        {
            query = getenv("QUERY_STRING");

            // 获取url上的命令参数
            ret = get_query_data(query,"cmd",cmd);
            if(ret < 0)goto END;

            char user_name[USER_LEN] = {0};
            char token[TOKEN_LEN] = {0};
            int start = 0;
            int count = 0;

            // 获取json上的元素
            if(strcmp(cmd,"count") == 0)
                ret = get_json_data_info(user_name,token,NULL,NULL);
            else 
                ret = get_json_data_info(user_name,token,&start,&count);

            if(ret == -1)goto END;

            // 检测登录状态
            ret = verify_token(user_name,token);
            if(ret != 0)goto END;

            if(strcmp(cmd,"count") == 0)
                push_count_data(user_name);
            else 
            //获取用户文件信息 127.0.0.1:80/myfiles&cmd=normal
            //按下载量升序 127.0.0.1:80/myfiles?cmd=pvasc
            //按下载量降序127.0.0.1:80/myfiles?cmd=pvdesc
                ret = push_user_file_list(cmd,user_name,start,count);      // 获取文件信息列表
        }else ret = -1;
END:
        if(ret == -1)printf(respost_code("104",NULL,NULL));
        else if(ret == 1)printf(respost_code("101",NULL,NULL));         
        else if(ret == 2)printf(respost_code("102",NULL,NULL));         // 文件为空
    }
    return 0;
}

int get_json_data_info(char *user_name,char *token,int *start,int *count)
{
    int ret = 0;
    int len = 0;
    char buf[4096] = {0};
    cJSON *root = NULL;
    cJSON *obj = NULL;

    len = fread(buf,1,4096,stdin);
    if(len <= 0)
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
    obj = cJSON_GetObjectItem(root,"user_name");
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

    if(start != NULL)
    {
        obj = cJSON_GetObjectItem(root,"start");
        if(obj == NULL)
        {
            ret = -1;
            goto END;
        }
        *start = obj->valueint;
    }

    if(count != NULL)
    {
        obj = cJSON_GetObjectItem(root,"count");
        if(obj == NULL)
        {
            ret = -1;
            goto END;
        }
        *count = obj->valueint;
    }

END:
    if(root != NULL)cJSON_Delete(root);
    return ret;
}


int push_count_data(const char *user_name)
{
    int ret = 0;
    char command[SQL_COM_LEN] = {0};
    char tmp[256] = {0};
    // 链接数据库
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }

    // 查询用户数据
    sprintf(command,"select count from user_file_count where user='%s'",user_name);
    ret = mysql_result_one(my,command,strlen(command),tmp);
    if(ret == -1)goto END;
    else if(ret == 0)   // 存在
        printf(respost_code("100","count",tmp)); 
    else                // 不存在 
    {
        printf(respost_code("100","count","0")); 
        ret = 0;
    }

END:
    if(my != NULL)mysql_conn_close(my);
    return ret;
}

int push_user_file_list(const char *cmd,const char *user_name,int start,int count)
{
    int ret = 0;
    char command[SQL_COM_LEN] = {0};
    MYSQL *my = NULL;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;

    cJSON *root = NULL;
    cJSON *arry = NULL;

    char *out = NULL;
    // 初始化数据库
    my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }
    // mysql查询
    if(strcmp(cmd,"normal") == 0)
        sprintf(command,"select user_file_list.*,file_info.url,file_info.size,file_info.type from file_info,user_file_list where user='%s' and file_info.md5=user_file_list.md5 limit %d,%d",user_name,start,count);
    else if(strcmp(cmd,"pvasc") == 0)
        sprintf(command,"select user_file_list.*,file_info.url,file_info.size,file_info.type from user_file_list,file_info where user='%s' and file_info.md5=user_file_list.md5 order by pv asc limit %d,%d",user_name,start,count);
    else if(strcmp(cmd,"pvdesc") == 0)
        sprintf(command,"select user_file_list.*,file_info.url,file_info.size,file_info.type from user_file_list,file_info where user='%s' and file_info.md5=user_file_list.md5 order by pv desc limit %d,%d",user_name,start,count);
    if(mysql_query(my,command) != 0)
    {
        ret = -1;
        goto END;
    }
    res = mysql_store_result(my);
    if(res == NULL)
    {
        ret = -1;
        goto END;
    }
    // 获取查询数据的长度
    long len = mysql_num_rows(res);
    if(len <= 0)        // 没有结果
    {
        ret = len == 0 ? 2 : -1;
        goto END;
    }
    
    root = cJSON_CreateObject();    // 创建json根文件
    arry = cJSON_CreateArray();     // 创建json数组文件
    while((row = mysql_fetch_row(res)) != NULL)
    {
        cJSON *obj = cJSON_CreateObject();

        int num = 1;
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"user",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"md5",row[num++]);       
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"create_time",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"file_name",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"share_status",row[num++]);
        if(row[num] != NULL)
            cJSON_AddNumberToObject(obj,"pv",atoi(row[num++]));
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"url",row[num++]);
        if(row[num] != NULL)
            cJSON_AddNumberToObject(obj,"size",atoi(row[num++]));
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"type",row[num++]);

        cJSON_AddItemToArray(arry,obj);
    }
    cJSON_AddStringToObject(root,"retcode","100");
    cJSON_AddItemToObject(root,"files",arry);

    out = cJSON_Print(root);
    if(out != NULL)
    {
        printf("%s",out);
        free(out);
    }
END:
    if(my != NULL)mysql_conn_close(my);
    if(res != NULL)mysql_free_result(res);
    if(root != NULL)cJSON_Delete(root);
    return ret;
}
//gcc -o gainfile_cgi gainfile_cgi.c ../cJSON.c ../cfg.c ../mysql_c_aip.c ../redis_c_api.c -I ../../include/ -lmysqlclient -lhiredis -lfcgi

