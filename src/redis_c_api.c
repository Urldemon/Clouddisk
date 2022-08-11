#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "../include/redis_c_api.h"
#include "../include/cfg.h"
#include "../include/define.h"


redisContext *redis_conn_init()
{
    char rd_ip[IP_LEN] = {0}; 
    char rd_port[PORT_SIZE] = {0};
    // 获取redis链接地址和接口
    get_conf_value(CONF_PATH,"redis","ip",rd_ip);
    get_conf_value(CONF_PATH,"redis","port",rd_port);

    // 建立链接
    redisContext *conn = redisConnect(rd_ip,atoi(rd_port));
    if(conn->err){
        redisFree(conn);
        conn = NULL;
    }
    return conn;
}

int redis_set_dbnum(redisContext *rd,int db_num){
    int ret = 0;
    redisReply* res = NULL;
    res = redisCommand(rd,"select %d",db_num);
    if(res == NULL)
    {
        ret = -1;
        goto END;
    }
END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}

int redis_setex_string(redisContext *rd,char *username,int time,char *token)
{
    redisReply * res = NULL;
    res = redisCommand(rd,"SETEX %s %d %s",username,time,token);
    if(res == NULL)
    {
        return -1;
    }
    freeReplyObject(res);
    return 0;
}

void redis_conn_free(redisContext *rd){
    redisFree(rd);
}

int redis_get_value(redisContext *rd,const char *key,char *out)
{
    redisReply *res = NULL;
    res = redisCommand(rd,"get %s",key);
    if(res->type == REDIS_REPLY_NIL)return -1;
    strcpy(out,res->str);
    freeReplyObject(res);
    return 0;
}
