#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"
#include "define.h"
#include "redis_c_api.h"
#include "cfg.h"

int get_conf_value(const char *jsonfile,const char *objname,const char *key, char *value){
    int ret = 0;
    long size = 0;
    char *buf = NULL;
    FILE * fp = NULL;
    int len = 0;

    if(objname == NULL || key == NULL || value == NULL)return -1;
    if(jsonfile == NULL)jsonfile = CONF_PATH;

    // 读方式打开conf
     fp = fopen(jsonfile,"r");
    if(fp == NULL)
    {
        ret = -1;
        goto END;
    }
   
    fseek(fp,0,SEEK_END);       // 光标移动到末尾
    size = ftell(fp);           // 计算文件大小
    fseek(fp,0,SEEK_SET);       // 光标移动到头

    buf = (char *)malloc(size+1);         // 开辟空间
    if(buf == NULL)
    {
        ret = -1;
        goto END;
    }

    // 读取文件内容
    len = fread(buf,1,size,fp);
    if(len < 0)
    {
        ret = -1;
        goto END;
    }
    buf[len] = '\0';

    // ==============json解析文件信息=======================
    // 将数据转化成json对象,内置分配空间
    cJSON *root = cJSON_Parse(buf);
    if(root == NULL)
    {
        ret = -1;
        goto END;
    }

    // 获取json中的指定对象
    cJSON *obj = cJSON_GetObjectItem(root,objname);
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    // 获取值
    cJSON *data = cJSON_GetObjectItem(obj,key);
    if(data == NULL)
    {
        ret = -1;
        goto END;
    }

    // 将至复制到value
    strcpy(value,data->valuestring); 

END:
    // 关闭fp
    if(fp != NULL)fclose(fp);
    // 释放buf空间
    if(buf != NULL)free(buf);
    // 关闭上方json空间
    if(root != NULL)cJSON_Delete(root);
    return ret;
}


int get_mysql_data(char *user,char *password, char *database){
    int ret = 0;
    FILE *fp = NULL;
    int size = 0;
    char *buf = NULL;
    if(user == NULL || password == NULL || database == NULL)
    {
       ret = -1; 
       goto END;
    }
    // 读方式打开conf
    fp = fopen(CONF_PATH,"r");
    if(fp == NULL)
    {
        ret = -1;
        goto END;
    }
   
    fseek(fp,0,SEEK_END);       // 光标移动到末尾
    size = ftell(fp);           // 计算文件大小
    fseek(fp,0,SEEK_SET);       // 光标移动到头

    buf = (char *)malloc(size+1);         // 开辟空间
    if(buf == NULL)
    {
        ret = -1;
        goto END;
    }

    // 读取文件内容
    int len = fread(buf,1,size,fp);
    if(len < 0)
    {
        ret = -1;
        goto END;
    }
    buf[len] = '\0';

    // 将数据转化成json对象,内置分配空间
    cJSON *root = cJSON_Parse(buf);
    if(root == NULL)
    {
        ret = -1;
        goto END;
    }

    // 获取json中的指定对象
    cJSON *obj = cJSON_GetObjectItem(root,"mysql");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }

    // 获取值 用户名
    strcpy(user,cJSON_GetObjectItem(obj,"user")->valuestring);
    // 获取用户密码 
    strcpy(password,cJSON_GetObjectItem(obj,"password")->valuestring);
    // 获取数据库名称
    strcpy(database,cJSON_GetObjectItem(obj,"database")->valuestring);

END:
    // 关闭fp
    if(fp != NULL)fclose(fp);
    // 释放buf空间
    if(buf != NULL)free(buf);
    // 关闭上方json空间
    if(root != NULL)cJSON_Delete(root);
    return ret;
}

int get_conf_inet_value(char *objname, char *ip, char *port)
{
    int ret = 0;
    FILE *fp = NULL;
    char *buf = NULL;
    int size = 0;
    if(objname == NULL || ip == NULL || port == NULL)
    {
       ret = -1; 
       goto END;
    }
    // 打开配置文件
    fp = fopen(CONF_PATH,"r"); 
    if(fp == NULL)
    {
        ret = -1;
        goto END;
    }
     // 计算大小
    fseek(fp,0,SEEK_END);
    size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    // 读取数据
    buf = (char *)malloc(size+1);
    if(buf == NULL)
    {
        ret = -1;
        goto END;
    }

    int len = fread(buf,1,size,fp);
    if(len < 0)
    {
        ret = -1;
        goto END;
    }

    // 将数据转化成json对象
    cJSON * root = cJSON_Parse(buf);
    if(root == NULL)
    {
        ret = -1;
        goto END;
    }
    // 获取数据对象
    cJSON * obj = cJSON_GetObjectItem(root,objname);
    if(obj == NULL)
    {
        ret  = -1;
        goto END;
    }
    // 将数据获取出来
    strcpy(ip,cJSON_GetObjectItem(obj,"ip")->valuestring);
    strcpy(port,cJSON_GetObjectItem(obj,"port")->valuestring);
END:
    if(fp != NULL)fclose(fp);
    if(root != NULL)cJSON_free(root);
    if(buf != NULL)free(buf);
    return ret;
}

int get_string_value(const char* str,const char *obj,char *value)
{
    cJSON *root = cJSON_Parse(str);
    if(root == NULL)
    {
        cJSON_free(root);
        return -1;
    }
    strcpy(value,cJSON_GetObjectItem(root,obj)->valuestring);
    cJSON_free(root);
    return 0;
}

char *respost_code(char *token,char *ret)
{
    cJSON *root_obj = cJSON_CreateObject();         // 创建一个对象

    cJSON_AddStringToObject(root_obj,"retcode",ret);    // 添加状态
    if(token != NULL)cJSON_AddStringToObject(root_obj,"token",token);    // 添加token值

    char *out = cJSON_Print(root_obj);      // 生成json数据

    if(root_obj != NULL)cJSON_Delete(root_obj);
    if(out != NULL){
        return out;       // 输出数据
    }
    return NULL;
}

int verify_token(const char *user_name,const char *token)
{
    int ret = 0;
    // 链接redis
    redisContext *rd = redis_conn_init();
    if(rd == NULL)return -1;

    // 获取key指定的token
    char buf[TOKEN_LEN] = {0};
    if(redis_get_value(rd,user_name,buf) < 0)
    {
        ret = -1;
        goto END;
    }

    // 判断是否一致
    if(strcmp(token,buf) != 0)
    {
        ret = 1;
        goto END;
    }
END:
    redis_conn_free(rd);
    return ret;         // -1 报错 0正确 1不正确
}

int get_query_data(const char *query,const char *key,char *cmd)
{
    char *index = NULL;
    char *end = NULL;
    index = strstr(query,key);
    if(index == NULL)
    {
        return -1;
    }
    index += strlen(key);       // 跳过cmd
    ++index;                    // 跳过end 获得参数起始位置
    end = index;
    while('\0' != *end && '#' != *end && '&' != *end)end++;     // 查找 & # \0 末端
    strncpy(cmd,index,end-index);
    cmd[end-index] = '\0';
    return 0;
}


