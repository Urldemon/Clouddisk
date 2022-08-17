#include <hiredis/read.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "redis_keys.h"
#include "redis_c_api.h"
#include "cfg.h"
#include "define.h"

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
    res = redisCommand(rd,"setex %s %d %s",username,time,token);
    if(strcmp(res->str,"Ok") != 0)
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
int redis_zlexcount(redisContext *rd,const char *key,const char *member)
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
int redis_zadd(redisContext *rd,const char *key,int score,const char *value)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"zadd %s %d %s",key,score,value);
    if(res->integer != 1)ret = -1;

    if(res != NULL)freeReplyObject(res);
    return ret;
}

/* *
 * @brief 查询有序数据集的元素个数
 * 
 * @param rd        redis链接
 * @param key       zset表名
 *
 * @returns 
 *          成功 0  失败 -1
 * */
int redis_zcard(redisContext *rd,const char *key)
{
   int ret = 0; 
   redisReply *res = NULL;

   res = redisCommand(rd,"zcard %s",key);
   if(res == NULL)
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
 * @brief 获取有序数据集的数据
 * 
 * @param rd        redis链接
 * @param start     start
 * @param end       end
 * @param value     传入参数传入获取的数据 
 * @param num       数据的大小
 *
 * @returns 
 *          成功 0  失败 -1
 * */
int redis_zrevrange(redisContext *rd,const char *key,int start,int end,RVALUES values,int *num)
{
    int ret = 0;
    redisReply *res = NULL;
    int i = 0;

    res = redisCommand(rd,"zrevrange %s %d %d",key,start,end);
    if(res->type != REDIS_REPLY_ARRAY)
    {
        ret = -1;
        goto END;
    }

    *num = (res->elements > end - start + 1) ? end - start + 1 : res->elements;

    for(i = 0;i < *num;++i)
    {
        strncpy(values[i],res->element[i]->str,VALUES_ID_SIZE-1);
        values[i][VALUES_ID_SIZE-1] = 0;
    }

END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}


/* *
 * @brief 删除key
 * 
 * @param conn      redis链接
 * @param key       key名称
 *
 * @returns 
 *          成功 0  失败 -1
 * */
int redis_del_key(redisContext *rd,const char *key)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"del %s",key);
    if(res->type != REDIS_REPLY_INTEGER)
    {
        ret = -1;
        goto END;
    }

    if(res->integer > 0)ret = 0;
    else ret = -1;

END:
    if(res != NULL)freeReplyObject(res); 
    return ret;
}

/* *
 * @brief 删除key
 * 
 * @param conn      redis链接
 * @param hahs_key       hahs_key名称
 * @param key       键值队的键
 * @param value     键值队的值
 *
 * @returns 
 *          成功 0  失败 -1
 * */
int redis_hset(redisContext *rd,const char *hash_key,const char *key,const char *value)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"hset %s %s %s",hash_key,key,value);
    if(res == NULL || res->type != REDIS_REPLY_INTEGER)
    {
        ret = -1;
        goto END;
    }
END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}

/* *
 * @brief 获取有序数据集的排序数据socre
 * 
 * @param conn      redis链接
 * @param key       key名称
 * @param member    对象名称
 *
 * @returns 
 *          成功 socre数据  失败 -1
 * */
int redis_zscore(redisContext *rd,const char *key,const char *member)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"zscore %s %s",key,member);
    if(res->type != REDIS_REPLY_STRING)
    {
        ret = -1;
        goto END;
    }

    ret = atoi(res->str);
END:
    if(res != NULL)freeReplyObject(res); 
    return ret;
}
/* *
 * @brief 获取有序数据集的排序数据socre
 * 
 * @param conn          redis链接
 * @param hash_key      hash表名称
 * @param key           key名称
 * @param value         传入参数返回键值
 *
 * @returns 
 *          成功 socre数据  失败 -1
 * */
int redis_hget(redisContext *rd,const char *hash_key,const char *key,char *value)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"hget %s %s",hash_key,key);
    if(res == NULL || res->type != REDIS_REPLY_STRING)
    {
        ret = -1;
        goto END;
    }

    int len = res->len > VALUES_ID_SIZE ? VALUES_ID_SIZE : res->len;

    strncpy(value,res->str,len);

    value[len] = '\0';
END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}
/* *
 * @brief 删除有序集合中的指定数据
 * 
 * @param conn          redis链接
 * @param key           表名称
 * @param member        删除的对象
 *
 * @returns 
 *          成功 0      失败 -1
 * */
int redis_zrem(redisContext *rd, const char *key, const char *member)
{
    int ret = 0;
    redisReply *res = NULL;

    res = redisCommand(rd,"zrem %s %s",key,member);
    if(res->integer != 1)
    {
        ret = -1;
        goto END;
    }

END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}
/* *
 * @brief 删除hash表中的指定数据
 * 
 * @param conn          redis链接
 * @param hash_key      hash表名称
 * @param member        指定数据
 *
 * @returns 
 *          成功 socre数据  失败 -1
 * */
int redis_hdel(redisContext *rd, const char *hash_key, const char *key)
{
    int ret = 0;
    redisReply *res = NULL;
    
    res = redisCommand(rd,"hdel %s %s",hash_key,key);
    if(res->integer != 1)
    {
        ret = -1;
        goto END;
    }
END:
    if(res != NULL)freeReplyObject(res);
    return ret;
}
