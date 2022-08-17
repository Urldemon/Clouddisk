#ifndef _REDIS_C_API_
#define _REDIS_C_API_ 

#include <hiredis/hiredis.h>

#define VALUES_ID_SIZE           1024       /* redis value域字段长度 */
//数组指针类型，其变量指向 char[1024]
typedef char (*RVALUES)[VALUES_ID_SIZE];    /* redis 表存放批量value字符串数组类型 */

redisContext *redis_conn_init();
int redis_set_dbnum(redisContext *rd,int db_num);
void redis_conn_free(redisContext *rd);
int redis_setex_string(redisContext *rd,char *username,int time,char *token);
int redis_get_value(redisContext *rd,const char *key,char *out);
int redis_zlexcount(redisContext *rd,const char *key,const char *member);
int redis_zadd(redisContext *rd,const char *key,int score,const char *value);
int redis_zcard(redisContext *rd,const char *key);
int redis_zrevrange(redisContext *rd,const char *key,int start,int end,RVALUES values,int *num);
int redis_del_key(redisContext *rd,const char *key);
int redis_hset(redisContext *rd,const char *hash_key,const char *key,const char *value);
int redis_zscore(redisContext *rd,const char *key,const char *member);

int redis_hget(redisContext *rd,const char *hash_key,const char *key,char *value);
int redis_zrem(redisContext *rd,const char *key,const char *member);
int redis_hdel(redisContext *rd,const char *hash_key,const char *key);
#endif
