#include <hiredis/read.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "../include/redis_keys.h"
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


/* *
 * @brief 判断有序序列集中是否有成员
 * 
 * @param conn      redis链接
 * @param key       zset表名
 * @param member    zset成员名
 *
 * @returns 
 *      0 不存在  1 存在  -1出错
 * */
int redis_zset_key_exist(redisContext *rd,const char *key,const char *member)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"zlexcount %s [%s [%s",key,member,member);
    if(res->type != REDIS_REPLY_INTEGER)
    {
        ret = -1;
        goto END;
    }

    ret = res->integer;
END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}
/* *
 * @brief 向有序集合key添加数据
 * 
 * @param conn      redis链接
 * @param key       zset表名
 * @param score     集合组
 * @param value     数据
 *
 * @returns 
 *          成功 0  失败 -1
 * */
int redis_zset_add(redisContext *rd,const char *key,int score,const char *value)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"zadd %s %d %s",key,score,value);
    if(res->integer != 1)ret = -1;

    if(res != NULL)freeReplyObject(res);
    return ret;
}

