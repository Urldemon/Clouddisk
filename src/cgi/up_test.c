#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "../../include/mysql_c_aip.h"
#include "../../include/define.h"
#include "../../include/redis_c_api.h"
#include <sys/socket.h>

int get_count_data(const char *user_name,int *count)
{
    int ret = 0;
    char command[SQL_COM_LEN] = {0};
    MYSQL_RES *res = NULL;    
    MYSQL_ROW row;

    MYSQL *my = mysql_conn_init();
    if(my == NULL)return -1;

    // 查询count数据并返回数据
    sprintf(command,"select count from user_file_count where user='%s'",user_name); 
    ret = mysql_query(my,command);
    if(ret == -1)goto END;

    res = mysql_store_result(my);
    if(res == NULL)goto END;
    
    // 获取返回的数据
    row = mysql_fetch_row(res);
    if(row == NULL || row[0] == NULL)goto END;

    // 转换字符
    *count = atoi(row[0]);

END:
    if(res != NULL)mysql_free_result(res);
    if(my != NULL)mysql_conn_close(my);
    return ret;            
}

int main(){
     
    int count = 0;
    get_count_data("15914108919",&count);
    printf("%d\n",count);
    return 0;
}
