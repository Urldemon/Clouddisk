#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "../../include/cfg.h"
#include "../../include/define.h"
#include "../../include/mysql_c_aip.h"
/*
 * 文件已经存在 005
 * 秒传成功 006
 * 秒传失败 007
 * 认证登录失败 111
 * 出现错误 404
 * */

int get_json_data(char *user_name,char *token,char *file_name,char *md5);
int deal_md5(char *user_name,char *md5,char *file_name);

int main(){
    while(FCGI_Accept() >= 0){
        int ret = 0;
        char *conlenght = NULL;

        char user_name[USER_LEN] = {0};
        char token[TOKEN_LEN] = {0};
        char file_name[FILE_NAME_LEN] = {0};
        char md5[MD5_LEN] = {0};

        conlenght = getenv("CONTENT_LENGTH");
        printf("Content-type: text/html\r\n\r\n");

        if(conlenght != NULL && atoi(conlenght) > 0){

            // 获得json文件 
            ret = get_json_data(user_name,token,file_name,md5);
            if(ret != 0)goto END;

            // 验证登录
            ret = verify_token(user_name,token);    // 1表示认证失败 -1表示出错 0表示成功
            if(ret != 0)goto END;

            // 处理秒传
            ret = deal_md5(user_name, md5, file_name); //秒传处理
        }else ret = -1;
END:
        if(ret == 0)printf(respost_code("006",NULL,NULL));              // 秒传成功
        else if(ret == 1)printf(respost_code("111",NULL,NULL));         // 认证登录失败
        else if(ret == 2)printf(respost_code("005",NULL,NULL));         // 文件已存在
        else if(ret == 3)printf(respost_code("007",NULL,NULL));         // 秒传失败
        else if(ret == -1)printf(respost_code("404",NULL,NULL));        // 出现错误
    }
    return 0;
}

int get_json_data(char *user_name,char *token,char *file_name,char *md5){
    int ret  = 0;
    char buf[4096] = {0};
    int len = fread(buf,1,4096,stdin);
    if(len <= 0)
    {
        ret = -1;
        goto END;
    }
    if(get_string_value(buf,"user_name",user_name) < 0){
        ret = -1;
        goto END;
    }
    if(get_string_value(buf,"token",token) < 0){
        ret = -1;
        goto END;
    }
    if(get_string_value(buf,"file_name",file_name) < 0){
        ret = -1;
        goto END;
    }
    if(get_string_value(buf,"md5",md5) < 0){
        ret = -1;
        goto END;
    }
END:
    return ret;
}

int deal_md5(char *user_name,char *md5,char *file_name){
    int ret = 0;
    int ret2 =  0;
    MYSQL *my = NULL;
    char command[SQL_COM_LEN] = {0};
    char count= 0;
    char create_time[TIME_LEN] = {0};
    MYSQL_RES *res = NULL;
    // 链接mysql
    my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }
    // 开启事务
    ret = mysql_conn_query(my,"begin",strlen("begin"),NULL);
    if(ret == -1)goto END;

    // 查询文件列表中是否存在md5文件并返回count值
    sprintf(command,"select count from file_info where md5='%s'",md5);
    ret2 = mysql_real_query(my,command,strlen(command));
    if(ret2 == -1){
        ret = -1;
        goto END;
    }

    res = mysql_store_result(my);
    if(res == NULL){
        ret = -1;
        goto END;
    }

    int line = mysql_num_rows(res);
    if(line == 0){
        ret = 3;                    // 没有记录显示秒传失败
        goto END;
    }

    MYSQL_ROW row;
    row = mysql_fetch_row(res);         // 从结果中取出一行,并放到结构体上
    if(row != NULL && row[0] != NULL)
        count =  atoi(row[0]);                  // 记录当前文件被用户依赖个数

    // 判断当前用户是已经存在该文件了
    memset(command,0,sizeof(command));
    sprintf(command,"select * from user_file_list where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
    ret2 = mysql_result_one(my,command,strlen(command),NULL);
    if(ret2 != 1){
        if(ret2 == -1)ret = -1;                                                     // 返回-1时出错                                             
        else ret = 2;                                                               // 当返回值ret2==0时,表示文件存在
        goto END;
    }

    // 修改当前文件依赖个数
    memset(command,0,sizeof(command));
    sprintf(command,"update file_info set count=%d where md5='%s'",count+1,md5);
    ret2 = mysql_conn_query(my,command,strlen(command),NULL);
    if(ret2 == -1){
        ret = -1;
        goto END;
    }

    // 文件存在执行秒传
    // 创建当前时间
    time_t now = time(NULL);
    strftime(create_time,TIME_LEN-1,"%Y-%m-%d %H:%M:%S",localtime(&now));

    // 将数据写入用户列表中
    memset(command,0,sizeof(command));
    sprintf(command,"insert into user_file_list (user,md5,create_time,file_name,shared_status,pv)\
            values ('%s','%s','%s','%s',0,0)",user_name,md5,create_time,file_name);

    ret2 = mysql_conn_query(my,command,strlen(command),NULL);
    if(ret2 == -1){
        ret = -1;
        goto END;
    }

    // 修改用户文件数量
    char buf[32] = {0}; 
    memset(command,0,sizeof(command));
    sprintf(command,"select count from user_file_count where user='%s'",user_name);

    ret2 = mysql_result_one(my,command,strlen(command),buf);
    memset(command,0,sizeof(command));
    if(ret2 == 0){
        sprintf(command,"update user_file_count set count=%d where user='%s'",atoi(buf)+1,user_name);
    }else if(ret2 == 1){
        sprintf(command,"insert into user_file_count (user,count) values ('%s',1)",user_name);
    }else{
        ret = -1;
        goto END;
    }

    ret2 = mysql_conn_query(my,command,strlen(command),NULL);
    if(ret2 == -1){
        ret = -1;
        goto END;
    }

END:
    // 事务处理
    if(ret2 == 0)mysql_conn_query(my,"commit",strlen("commit"),NULL);
    else mysql_conn_query(my,"rollback",strlen("rollback"),NULL);

    if(res != NULL)mysql_free_result(res);
    if(my != NULL)mysql_conn_close(my);
    return ret;
}
