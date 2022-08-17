#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "fcgi_stdio.h"
#include "fcgi_config.h"
#include "cfg.h"
#include "cJSON.h"
#include "mysql_c_aip.h"


int get_json_data(char *user_name,char *nick_name,char *password,char *phone,char *email)
{
    int ret = 0;
    char buf[4096] = {0};
    // 读取http数据端
    int rlen = fread(buf,1,4096,stdin); 
    if(rlen <= 0)
    {
        ret = -1;
        goto END;
    }

    cJSON *root = cJSON_Parse(buf); 
    // 拷贝数据
    cJSON *obj = cJSON_GetObjectItem(root,"user_name");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(user_name,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"nick_name");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(nick_name,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"password");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(password,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"phone");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(phone,obj->valuestring);

    obj = cJSON_GetObjectItem(root,"email");
    if(obj == NULL)
    {
        ret = -1;
        goto END;
    }
    strcpy(email,obj->valuestring);

END:
    if(root != NULL)cJSON_Delete(root);
    return ret;
}

int user_register(const char *user_name,const char *nick_name,const char *password,const char *email,const char *phone){
    int ret = 0;
    char command[SQL_COM_LEN] = {0};
    char buf[512] = {0};
    char create_time[TIME_LEN]= {0};

    // 链接数据库
    MYSQL *my = mysql_conn_init();

    // 查询用户是否存在
    sprintf(command,"select * from user_info where user_name='%s'",user_name);
    ret = mysql_result_one(my, command,strlen(command),NULL); 
    if(ret != 1)
    {
        if(ret != -1)ret = 1;
        goto END;
    }

    
    // 查看用户的手机是否为空
    memset(command,0,sizeof(command));
    sprintf(command,"select * from user_info where phone='%s'",phone);
    ret = mysql_result_one(my, command,strlen(command),NULL); 
    if(ret != 1)
    {
        if(ret != -1)ret = 2;
        goto END;
    }

    // 查看用户的邮箱是否为被注册
    memset(command,0,sizeof(command));
    sprintf(command,"select * from user_info where email='%s'",email);
    ret = mysql_result_one(my, command,strlen(command),NULL); 
    if(ret != 1)
    {
        if(ret != -1)ret = 3;
        goto END;
    }

    // nick_name 是否重复 
    sprintf(command,"select * from user_info where nick_name='%s'",nick_name);
    ret = mysql_result_one(my, command,strlen(command),NULL); 
    if(ret != 1)
    {
        if(ret != -1)ret = 4;
        goto END;
    }

    // 获取当前时间
    time_t now = time(NULL);
    strftime(create_time,TIME_LEN-1,"%Y-%m-%d %H:%M:%S",localtime(&now));

    // 将数据写入mysql用户列表
    memset(command,0,sizeof(command));
    sprintf(command,"insert into user_info (user_name,nick_name,password,phone,email,create_time)values('%s','%s','%s','%s','%s','%s')",user_name,nick_name,password,phone,email,create_time);

    ret = mysql_conn_query(my,command,strlen(command),NULL);
END:
    if(my != NULL)mysql_conn_close(my);
    return ret;
}
int main(){
    while(FCGI_Accept() >= 0){
        int ret = 0;
        char *out = NULL;

        char *conlength= getenv("CONTENT_LENGTH");
        int conlen = 0;
        if(conlength != NULL)
        {
            conlen = atoi(conlength);
        }
        printf("Content-Type: text/html;charset=utf-8;\r\n\r\n");
        if(conlen > 0)
        {
            // 将获取的json格式数据进行拆分
            char user_name[USER_LEN] = {0};
            char nick_name[USER_LEN] = {0};
            char password[PASSWORD_LEN] = {0};
            char phone[PASSWORD_LEN] = {0};
            char email[PASSWORD_LEN] = {0};

            // 获取数据
            if(get_json_data(user_name,nick_name,password,phone,email) < 0)
            {
                ret = -1;
                goto END;
            }

            // 注册操作
            ret = user_register(user_name,nick_name,password,email,phone);

        }

END:

        switch (ret) {
            case 0:
                printf(respost_code("010",NULL,NULL)); 
                break;
            case 1:
                printf(respost_code("011",NULL,NULL)); 
                break;
            case 2:
                printf(respost_code("012",NULL,NULL)); 
                break;
            case 3:
                printf(respost_code("013",NULL,NULL)); 
                break;
            case 4:
                printf(respost_code("014",NULL,NULL)); 
                break;
            default:
                printf(respost_code("015",NULL,NULL)); 
                break;
        }
    }
    return 0;
}

// gcc -o reup_cgi reup_cgi.c ../cfg.c ../cJSON.c ../mysql_c_aip.c ../redis_c_api.c -I ../../include/ -lfcgi -lmysqlclient -lhiredis

