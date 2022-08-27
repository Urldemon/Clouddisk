#include <alloca.h>
#include <hiredis/hiredis.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <wchar.h>
#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "define.h"
#include "mysql_c_aip.h"
#include "redis_c_api.h"
#include "redis_keys.h"
#include "cfg.h"
#include "cJSON.h"

/*
 * 100  操作成功
 * 101  验证登录失败
 * 102  没有文件
 * */
int get_json_data_info(char *user_name,char *token,int *start,int *count);
int push_share_count();  
int push_file(int start,int count);
int push_file_ranking_pv(int start,int count);
int main()
{
    while (FCGI_Accept() >= 0) {
        int ret = 0;
        char *conlength = NULL;
        char *query = NULL;

        char cmd[20] = {0};
        char user_name[USER_LEN] = {0};
        char token[TOKEN_LEN] = {0};
        int count = 0;
        int start = 0;

        printf("Content-Type: text/html\r\n\r\n"); 
        conlength = getenv("CONTENT_LENGTH");

        if(conlength != NULL && atoi(conlength) > 0)
        {
            // 获取url上的命令数据
            query = getenv("QUERY_STRING");
            ret = get_query_data(query,"cmd",cmd);
            if(ret == -1)
            {
                ret = -1;
                goto END;
            }

            // 获取http上传输的数据
            if(strcmp(cmd,"count") == 0)
                ret = get_json_data_info(user_name,token,NULL,NULL);
            else 
                ret = get_json_data_info(user_name,token,&start,&count);

            if(ret == -1)goto END;

            // 验证登录
            ret = verify_token(user_name,token);
            if(ret != 0)goto END;

            // 执行命令操作
            if(strcmp(cmd,"count") == 0)        // 获取共享列表数量
                ret = push_share_count();  
            else if(strcmp(cmd,"normal") == 0)      // 获取共享列表
                ret = push_file(start,count);    
            else if(strcmp(cmd,"pvdesc") == 0)           // 获取排序列表从start开始的后count个
                ret = push_file_ranking_pv(start,count);  
            else ret = -1;
        }else ret = -1; 

END:
        if(ret == -1)printf(respost_code("104",NULL,NULL));      // 出错
        else if(ret == 1)printf(respost_code("101",NULL,NULL));  // 验证失败
        else if(ret == 2)printf(respost_code("102",NULL,NULL));  // 没有文件
    }
    return 0;
}


int get_json_data_info(char *user_name,char *token,int *start,int *count)
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
    
    if(start != NULL)
    {
        obj = cJSON_GetObjectItem(root,"count");
        if(obj == NULL)
        {
            ret = -1;
            goto END;
        }
        *count = obj->valueint;
    }
    ret = 0;
END:
    if(root != NULL)cJSON_Delete(root);
    return ret;
}

int push_share_count()
{
    int ret = 0;
    char *out = NULL;
    char command[SQL_COM_LEN] = {0}; 
    char tmp[256] = {0};
    // 链接数据库
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }

    // 获取共享文件的数量
    sprintf(command,"select count from user_file_count where user='shared_count'");
    ret = mysql_result_one(my,command,strlen(command),tmp);
    if(ret == -1)
        goto END;
    else if(ret == 0)
        printf(respost_code("100","count",tmp));
    else if(ret == 1)
    {
        printf(respost_code("100","count","0"));
        ret = 0;
    }

END:
    if(my != NULL)mysql_conn_close(my);
    return ret;
}

int push_file(int start,int count)
{
    int ret = 0;
    char command[SQL_COM_LEN] = {0};

    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    ulong line = 0;


    cJSON *root = NULL;
    cJSON *arry = NULL;

    char *out = NULL;

    // 链接mysql
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        printf("1\n");
        ret = -1;
        goto END;
    }
    
    // 链接redis
    redisContext *rd = redis_conn_init();
    if(rd == NULL)
    {
        ret = -1;
        goto END;
    }
   
    // 遍历MySQL上的数据并写入cJSON
    memset(command,0,sizeof(command));
    sprintf(command,"select share_file_list.*,file_info.url,file_info.size,file_info.type from share_file_list,file_info where file_info.md5=share_file_list.md5 limit %d,%d",start,count); 
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret = -1;
        goto END;
    }
    // 获取my上的数据
    res = mysql_store_result(my);
    if(res == NULL)
    {
        ret = -1;
        goto END;
    }
    // 查看是数据个数
    line = mysql_num_rows(res);
    if(line <= 0)
    {
        ret = line == 0 ? 2 : -1;
        goto END;
    }
    // 创建cJSON对象
    root = cJSON_CreateObject();
    arry = cJSON_CreateArray();

    while((row = mysql_fetch_row(res)) != NULL)
    {
        /*
        {
        "user": "milo",
        "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        "create_time": "2020-06-21 21:35:25",
        "file_name": "test.mp4",
        "share_status": 1,
        "pv": 0,
        "url": "http://192.168.31.109:80/group1/M00/00/00/wKgfbViy2Z2AJ-FTAaM3As-g3Z0782.mp4",
        "size": 27473666,
         "type": "mp4"
        }
        */
        cJSON *obj = cJSON_CreateObject();

        int num= 1;
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"user_name",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"md5",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"file_name",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"pv",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"create_time",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"url",row[num++]);
        if(row[num] != NULL)
            cJSON_AddStringToObject(obj,"size",row[num++]);
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

int push_file_ranking_pv(int start,int count)
{
    int ret = 0;
    char command[SQL_COM_LEN] = {0};
    char tmp[256] = {0};
    RVALUES values = NULL;
    int line = 0;
    int i = 0;

    MYSQL_RES *res = NULL;
    MYSQL_ROW row;

    cJSON *root = NULL;
    cJSON *arry = NULL;
    char *out = NULL;

    // 链接mysql
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        printf("1\n");
        ret = -1;
        goto END;
    }
    
    // 链接redis
    redisContext *rd = redis_conn_init();
    if(rd == NULL)
    {
        ret = -1;
        goto END;
    }

    // 查询redis上是否有缓存
    int redis_num = redis_zcard(rd,FILE_PUBLIC_ZSET); 
    if(redis_num == -1)
    {
        ret = -1;
        goto END;
    }

    // 查询mysql上的数据个数
    int mysql_num = 0;
    sprintf(command,"select count from user_file_count where user='shared_count'");
    if(mysql_result_one(my,command,strlen(command),tmp) != 0) 
    {
        ret = -1;
        goto END;
    }
    mysql_num = atoi(tmp);

    // 查看mysql和redis数据是否不一致
    if(redis_num != mysql_num)
    {
        // 删除键 
        redis_del_key(rd,FILE_PUBLIC_ZSET);
        redis_del_key(rd,FILE_NAME_HASH);

        // 从mysql上获取数据并写入redis
        memset(command,0,sizeof(command));
        sprintf(command,"select md5,file_name,pv from share_file_list order by pv desc");
        if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
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
        while ((row = mysql_fetch_row(res)) != NULL) {
            if(row[0] == NULL || row[1] == NULL || row[2] == NULL)
            {
                ret = -1;
                goto END;
            }
            char fileid[1024] = {0};
            sprintf(fileid,"%s%s",row[0],row[1]);

            // 添加数据集到有序数据集合中
            redis_zadd(rd,FILE_PUBLIC_ZSET,atoi(row[2]), fileid);

            // 将文件的md5写入哈希表中
            redis_hset(rd,FILE_NAME_HASH,fileid,row[0]);
        }
    }

    // 将redis上的数据返回给前端
    values = (RVALUES)calloc(count,VALUES_ID_SIZE);
    if(values == NULL)
    {
        ret = -1;
        goto END;
    }
    
    // 获取redis上的数据
    ret = redis_zrevrange(rd,FILE_PUBLIC_ZSET,start,count,values,&line);
    if(ret == -1)goto END;

    // 将数据转化成json数据
    root = cJSON_CreateObject();
    arry = cJSON_CreateArray();

    for(i = 0;i < line;++i)
    {
        cJSON *item = cJSON_CreateObject();

        char file_name[FILE_NAME_LEN] = {0};

        // 根据fileid获取file_name
        ret = redis_hget(rd,FILE_NAME_HASH,values[i],file_name);
        if(ret == -1)goto END;
        cJSON_AddStringToObject(item,"file_name",file_name);

        // 获取pv下载数量
        int pv = redis_zscore(rd,FILE_PUBLIC_ZSET,values[i]);
        if(pv == -1)
        {
            ret = -1;
            goto END;
        }
        cJSON_AddNumberToObject(item,"pv",(const double)pv);

        cJSON_AddItemToArray(arry,item);
    }
    cJSON_AddStringToObject(root,"retcode","100");
    cJSON_AddItemToObject(root,"files",arry);

    out = cJSON_Print(root);
    if(out != NULL && ret == 0)
    {
        printf("%s",out);
        free(out);
    }
END:
    if(my != NULL)mysql_conn_close(my);
    if(res != NULL)mysql_free_result(res);
    if(rd != NULL)redis_conn_free(rd);
    if(values != NULL)free(values);
    if(root != NULL)cJSON_Delete(root);
    return ret;
}

//gcc -o sharefile_cgi sharefile_cgi.c ../cJSON.c  ../cfg.c ../redis_c_api.c ../mysql_c_aip.c -I ../../include/ -lhiredis -lmysqlclient -lfcgi 
