#ifndef _MYSQL_C_API_
#define _MYSQL_C_API_

#include <mysql/mysql.h>
MYSQL *mysql_conn_init();
int mysql_conn_query(MYSQL *my,const char *command,int command_size,MYSQL_RES *ret_data);
void mysql_conn_close(MYSQL *my);

int mysql_result_one(MYSQL *my,const char *command,long command_size,char *buf);
#endif
