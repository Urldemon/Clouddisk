#include <hiredis/hiredis.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include "../../include/define.h"
#include "../../include/redis_c_api.h"
#include "../../include/redis_keys.h"
#include "../../include/mysql_c_aip.h"
#include "../../include/fdfs_c_api.h"
#include "../../include/cfg.h"
#include "../../include/cJSON.h"
/*
 * 110 操作成功
 * 111 登录验证失败
 * 112 共享已存在
 * 113 无文件需要删除
 * */

int get_json_info(char *user_name,char *md5,char *token,char *file_name);
void del_file(char *user_name,char *md5,char *file_name);
void pv_file(char *user_name,char *md5,char *file_name);
void share_file(char *user_name,char *md5,char *file_name);

int main()
{
    while(FCGI_Accept() >= 0)
    {
        int ret = 0;
        char *conlength = NULL;
        char *query = NULL;
        char cmd[CMD_LEN] = {0};

        printf("Content-Type: text/html;charset=utf-8;\r\n\r\n");

        conlength = getenv("CONTENT_LENGTH");
        if(conlength != NULL && atoi(conlength) > 0)
        {
            query = getenv("QUERY_STRING");

            // 获取url上的命令参数
            ret = get_query_data(query,"cmd",cmd);
            if(ret < 0)goto END;

            char user_name[USER_LEN] = {0};
            char md5[MD5_LEN] = {0};
            char token[TOKEN_LEN] = {0};
            char file_name[FILE_NAME_LEN] = {0};

            // 获取前段发来的数据
            ret = get_json_info(user_name,md5,token,file_name);
            if(ret == -1)goto END;

            // 验证登录
            ret = verify_token(user_name,token);
            if(ret == -1)
            {
                ret = 1;
                goto END;
            }


            // 对url的指令进行划分
            if(strcmp(cmd,"del") == 0)
            {
                del_file(user_name,md5,file_name);
            }
            else if(strcmp(cmd,"pv") == 0)
            {
                pv_file(user_name,md5,file_name);
            }
            else if(strcmp(cmd,"share") == 0)
            {
                share_file(user_name,md5,file_name);
            }
        }else ret = -1;
END:
        if(ret == -1)printf(respost_code(NULL,"114"));              // 出错
        else if(ret == 1)printf(respost_code(NULL,"111"));          // 登录验证失败
        
    }
    return 0;
}


int get_json_info(char *user_name,char *md5,char *token,char *file_name)
{
    int len = 0;
    int ret = 0;
    char buf[4096] = {0};
    
    // 读取数据
    len = fread(buf,1,4096,stdin);
    if(len <= 0)return -1;

    // 拆分数据对象
    cJSON *root = cJSON_Parse(buf);
    if(root == NULL)return -1;

    cJSON *obj = cJSON_GetObjectItem(root,"user_name");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(user_name,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"md5");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(md5,obj->valuestring);
    
    obj = cJSON_GetObjectItem(root,"token");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(token,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"file_name");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(file_name,obj->valuestring);

END:
    if(root != NULL)cJSON_Delete(root);
    return ret;
}

// 用户文件删除存储文件操作
void del_file(char *user_name,char *md5,char *file_name)
{
    int ret = 0;        // 记录函数状态
    int ret2 = 0;       // 记录事务执行状态
    char command[SQL_COM_LEN] = {0};
	char tmp[256] = {0};
	char file_id[FILE_ID_LEN] = {0};
    char fdfs_cli_path[1024] = {0};

    // 获取fdfsclient客户端配置文件
    if(get_conf_value(CONF_PATH,"fastdfs","client_conf_path",fdfs_cli_path) != 0)
    {
        ret = -1;
        goto END;
    }

    // 链接数据库
    MYSQL *my = mysql_conn_init();

    if(my == NULL)
    {
        ret = -1;
        goto END;
    }

    // 获取file_id
	memset(command,0,sizeof(command));
	sprintf(command,"select file_id from file_info where md5='%s'",md5);
	if(mysql_result_one(my,command,strlen(command),file_id) != 0)
    {
        ret = -1;
        goto END;
    }

     //==== 开启事务 ==== 
	if(mysql_conn_query(my,"begin",strlen("begin"),NULL) != 0)
	{
		ret = -1;
		goto END;
    }

    // ===================== 删除mysql上的文件相关数据 ========================
    // 删除用户文件数量
    memset(command,0,sizeof(command));
    sprintf(command,"select count from user_file_count where user='%s'",user_name);
    if(mysql_result_one(my,command,strlen(command),tmp) != 0)
    {
        ret2 = -1;
        goto END;
    }
    
    if(atoi(tmp) > 0) // 用户存储的文件数量大于0时进行删减
    {  
        memset(command,0,sizeof(command));
        sprintf(command,"update user_file_count set count='%d' where user='%s'",atoi(tmp)-1,user_name);
        if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
        {
            ret2 = -1;
            goto END;
        }
    }else{          // 用户没有文件
        ret = 1;
        goto END;
    }

    // 查询共享目录并修改数据
    sprintf(command,"select shared_status from user_file_list where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
    if(mysql_result_one(my,command,strlen(command),tmp) != 0)
    {
        ret2 = -1;
        goto END;
    }
    if(atoi(tmp) == 1)
    {
        memset(command,0,sizeof(command)); 
        sprintf(command,"delete from share_file_list where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
        if(mysql_conn_query(my,command, strlen(command),NULL) != 0)
        {
            ret2 = -1;
            goto END;
        }
    }

    // 删除用户上文件信息的文件
    memset(command,0,sizeof(command));
    sprintf(command,"delete from user_file_list where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
    if(mysql_conn_query(my,command, strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }
    
    //============================删除文件信息上的数据，并删除fdfs下的文件数据 ==================================

	// 执行删除命令
	memset(command,0,sizeof(command));
	sprintf(command,"delete from file_info where md5='%s'",md5);
	if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }
	// 删除fdfs下的文件 ,删除失败则事务回滚
	if(fdfs_delete_file(fdfs_cli_path,file_id) != 0)
    {
        ret2 = -1;
        goto END;
    }

END:
	// ==== 提交事务/回滚事务 ====
    if(ret2 == 0) mysql_conn_query(my,"commit",strlen("commit"),NULL);
    else {
        mysql_conn_query(my,"rollback",strlen("rollback"),NULL);
        ret = -1;
    }

    if(ret == -1)
        printf(respost_code(NULL,"114"));      // 出错
    else if(ret == 0)
        printf(respost_code(NULL,"110"));       // 成功
    else if(ret == 1)
        printf(respost_code(NULL,"113"));       // 用户没有文件不能删除

    if(my != NULL)mysql_conn_close(my);
}

// 用户文件下载操作
void pv_file(char *user_name,char *md5,char *file_name)
{
    int ret = 0;
    char command[SQL_COM_LEN] = {0};
    char tmp[256] = {0};
    int pv = 0;

    // 链接mysql
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }

    // 查询下载记录
    sprintf(command,"select pv from user_file_list where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
    if(mysql_result_one(my,command,strlen(command),tmp) != 0)
    {
        ret = -1;
        goto END;
    }
    pv = atoi(tmp);
    
    // 修改下载数据
    memset(command,0,sizeof(command));
    sprintf(command,"update user_file_list set pv=%d where user='%s' and md5='%s' and file_name='%s'",pv+1,user_name,md5,file_name);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret = -1;
        goto END;
    }
END:
    if(my != NULL)mysql_conn_close(my);
    if(ret == 0)printf(respost_code(NULL,"110"));       // 操作成功
    else printf(respost_code(NULL,"114"));              // 出错
}

// 用户文件分享操作
void share_file(char *user_name,char *md5,char *file_name)
{
    int ret = 0;
    int ret2 = 0;
    char file_code[FILE_ID_LEN] = {0};
    char command[SQL_COM_LEN] = {0};
    char tmp[256] = {0};
    char create_time[25] = {0};

    // 链接mysql和redis数据库
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        ret = -1;
        goto END;
    }
    
    redisContext *rd = redis_conn_init();
    if(rd == NULL)
    {
        ret = -1;
        goto END;
    }

    // 设置文件标识 (md5+file_name)
    sprintf(file_code,"%s%s",md5,file_name); 

    // 先判断redis上是否有数据 (当有人调用则会再起显示)
    ret = redis_zset_key_exist(rd,FILE_PUBLIC_ZSET,file_code); 
    if(ret == 1)    // 存在
    {
        printf("1\n");
        ret = 1; 
        goto END;
    }
    else if(ret == 0)  // 不存在 ，去mysql上查看是否有
    {
        sprintf(command,"select * from share_file_list where md5='%s' and file_name='%s'",md5,file_name); 
        ret = mysql_result_one(my,command,strlen(command),NULL);
        if(ret == -1)     // 错误
        {
            ret = -1;   
            goto END;
        }else if(ret == 0)      // 存在
        {
            printf("2\n");
            redis_zset_add(rd,FILE_PUBLIC_ZSET,0,file_code);    // 将共享数据存入redis
            ret = 1;    
            goto END;
        }
    }
    else 
    {
        ret = -1;
        goto END;
    }

    ret = 0;
    // 当mysql的共享文件列表没有时
    // =========开启事务=========
    if(mysql_conn_query(my,"begin",strlen("begin"),NULL) != 0)
    {
        ret = -1;
        goto END;
    }

    // 将共享的文件共享标识置为1
    memset(command,0,sizeof(command));
    sprintf(command,"update user_file_list set shared_status = 1 where user='%s' and md5='%s' and file_name='%s'",user_name,md5,file_name);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }

    // 获取当前时间
    time_t now = time(NULL);
    strftime(create_time, TIME_LEN-1, "%Y-%m-%d %H:%M:%S", localtime(&now));

    // 将共享的文件信息传入共享文件；列表中
    memset(command,0,sizeof(command));
    sprintf(command,"insert into share_file_list (user, md5,file_name,create_time,pv) values ('%s', '%s', '%s', '%s', %d)",user_name,md5,file_name,create_time,0);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }

    // 修改共享文件的个数
    memset(command,0,sizeof(command));
    sprintf(command,"select count from user_file_count where user='shared_count'");
    if(mysql_result_one(my,command,strlen(command),tmp) != 0)
    {
        ret2 = -1;
        goto END;
    }

    memset(command,0,sizeof(command));
    sprintf(command,"update user_file_count set count= %d where user='shared_count'",atoi(tmp)+1);
    if(mysql_conn_query(my,command,strlen(command),NULL) != 0)
    {
        ret2 = -1;
        goto END;
    }
END:
    // ==========事务提交/回滚==========
    if(ret2 == 0)mysql_conn_query(my,"commit",strlen("commit"),NULL);
    else 
    {
        printf("end\n");
        mysql_conn_query(my,"rollback",strlen("rollback"),NULL);
        ret = -1;
    }

    if(ret == -1)printf(respost_code(NULL,"114"));          // 出错
    else if(ret == 0)printf(respost_code(NULL,"110"));      // 共享成功
    else if(ret == 1)printf(respost_code(NULL,"112"));      // 已经存在共享

}

// gcc -o ctl_cgi ctl_cgi.c ../cfg.c ../cJSON.c ../mysql_c_aip.c ../redis_c_api.c ../fdfs_c_api.c -I ../../include/ -I /usr/include/fastdfs/  -lmysqlclient -lfcgi -lfdfsclient -lhiredis


