#ifndef _REDIS_C_API_
#define _REDIS_C_API_ 

#include <hiredis/hiredis.h>

redisContext *redis_conn_init();
int redis_set_dbnum(redisContext *rd,int db_num);
void redis_conn_free(redisContext *rd);
int redis_setex_string(redisContext *rd,char *username,int time,char *token);
int redis_get_value(redisContext *rd,const char *key,char *out);
#endif
