#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include "define.h"
#include "cfg.h"
#include "mysql_c_aip.h"

MYSQL *mysql_conn_init()
{
    char ip[IP_LEN] = {0};
    char port_str[PORT_SIZE] = {0};
    char user[USER_LEN] = {0};
    char password[PASSWORD_LEN] = {0};
    char database[DATABASE_LEN] = {0};

    // 获取配置文件数据
    get_mysql_data(user,password,database);
    get_conf_inet_value("mysql",ip,port_str);
    int port = atoi(port_str);

    // 初始化mysql对象 
    MYSQL *mq = mysql_init(NULL);

    if(mq == NULL)
    {
        fprintf(stderr,"mysql 初始化失败\n");
        return NULL;
    }

    // 创建链接
    if(mysql_real_connect(mq,ip,user,password,database,port,NULL,0) == NULL)
    {
        fprintf(stderr,"mysql_conn_init 失败:ERROR %u(%s)\n",mysql_errno(mq),mysql_error(mq));
        mysql_close(mq);
        return NULL;
    }
    // 设置编码库
    mysql_query(mq,"set names utf8");
    return mq;
}

int mysql_conn_query(MYSQL *my,const char *command,long command_size,MYSQL_RES *ret_data)
{
    int ret = 0;
    if(my == NULL || command == NULL || command_size == 0)return -1;

    int res = mysql_real_query(my,command,command_size);
    if(res != 0)
    {
        printf("mysql_real_query ERROR:%s\n",mysql_error(my));
        return -1;
    }
    return ret;
}
void mysql_conn_close(MYSQL *my)
{
    mysql_close(my);
}

// 获取一个判断表中是否存在数据，只限定查找一个返回数据
// 查询数据是否在mysql中存在则返回0和对象数据,后续需要对数据进行修改，不存在则返回1
int mysql_result_one(MYSQL *my,const char *command,long command_size,char *buf)
{
    int ret = 0;

    // 查询数据
    if(mysql_real_query(my,command, command_size) < 0)
    {
        ret = -1;
        goto END;
    }

    // 将查询到的数据获取进行分析
    MYSQL_RES *res = NULL; 
    long line = 0;
    res = mysql_store_result(my);
    if(res == NULL){
        ret = -1;
        goto END;
    }
    // mysql_num_rows接受由mysql_store_result的结果，并返回结果中的行数
    line = mysql_num_rows(res);
    if(line == 0)
    {
        ret = 1;        // 没有记录
        goto END;
    }
    // mysql_fetch_row从结果中提取出一行，并放到一个结构体中
    MYSQL_ROW row;
    if((row = mysql_fetch_row(res)) != NULL)
    {
        if(row[0] != NULL)
        {
            if(buf != NULL)strcpy(buf,row[0]);
        }
    }
END:
    // 释放MYSQL_RES对象
    if(res != NULL)mysql_free_result(res);
    return ret;
}

