#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <wchar.h>
#include "define.h"
#include "cfg.h"
#include "mysql_c_aip.h"
#include "base64.h"
#include "cJSON.h"
#include "redis_c_api.h"



int check_user_pwd(char *user_name,char *pwd)
{
    int ret = 0;

    // 获取conf配置信息

    // 链接数据库
    MYSQL *my = mysql_conn_init();
    if(my == NULL)
    {
        return -1;
    }
    // 获取用户信息表的数据
    char command[SQL_COM_LEN] = {0};
    sprintf(command,"select password from user_info where user_name='%s'",user_name);

    // mysql_conn_query(my,command,strlen(command),res);
    char tmp[PASSWORD_LEN] = {0};

    // 查询账号的密码 
    ret = mysql_result_one(my,command,strlen(command),tmp);
    if(ret == 0){           // ret -1错误 ret 0登录成功 ret 1没有该用户 ret 2密码不正确
        if(strcmp(tmp,pwd) == 0){
            ret = 0;            
        }else ret = 2;         
    }

    mysql_conn_close(my);
    return ret;         // -1 错误 0 正确 1 不存在 2 密码不正确
}

int set_token(char *user_name,char *token)
{
    char buf[256] = {0};        // token未加密数据

    char aes_key[17];           // 获取aeskey
    unsigned char ivec[AES_BLOCK_SIZE] = {0};   // ivec初始化向量值

    char base64[1024*3] = {0};      // base64编码数据
    unsigned char sha256[SHA256_DIGEST_LENGTH] = {0};   // 生成的sha256编码 后面需要转化程16进制

    // 链接redis数据库
    redisContext * rd = redis_conn_init(); 
    if(rd == NULL)
    {
        return -1;
    }
    
    // 生成随机数有用户名加随机生成的4个数子组成
    // 设置种子 
    srand(time(NULL));
    int rand_num[4] = {0};
    int i = 0;
    for(;i < 4;++i){
        rand_num[i] = rand()%1000;
    }
    sprintf(buf,"%s%d%d%d%d",user_name,rand_num[0],rand_num[1],rand_num[2],rand_num[3]);

    // 对token数据进行对称分组加密加密
    AES_KEY enkey; 
    get_conf_value(CONF_PATH,"key","aes",aes_key);
    AES_set_encrypt_key((const unsigned char*)aes_key,16*8,&enkey);

    int len = strlen(buf)+1;
    if(len % 16 != 0)
    {
        len = ((len / 16)+1)*16;
    }

    char *out = (char *)malloc(len);
    AES_cbc_encrypt((const unsigned char*)buf,(unsigned char*)out,len,&enkey,ivec,AES_ENCRYPT);

    // base64转码
    base64_encode((const unsigned char *)out,len,base64);

    // 特殊情况 user_name进过256加密后自动变成了数字，通过提前备份实现保留名称
    char name[USER_LEN]={0};
    strcpy(name,user_name);

    // sha256加密
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    // 添加数据
    SHA256_Update(&ctx,base64,strlen(base64));
    // 加密
    SHA256_Final(sha256,&ctx);
    // 转化格式为16进制并存储到token
    for(i = 0;i < SHA256_DIGEST_LENGTH;++i)
    {
        sprintf(&token[i],"%02x",sha256[i]);
    }

    // 将token写入redis暂时缓存,填入临时备份数据
    redis_setex_string(rd,name,86400,token);

    // 释放资源
    redis_conn_free(rd);
    free(out);

    return 0; 
}



int main(){
    int ret = 0;
    
    
    while(FCGI_Accept() >= 0)
    {
        char *connlength = getenv("CONTENT_LENGTH"); 
        long connlen = 0;
        char token[SHA256_DIGEST_LENGTH] = {0};
        char *out = NULL;

        if(connlength != NULL)
        {
            connlen = strtol(connlength,NULL,10);
        }
        printf("Content_type: text/html\r\n\r\n");
        if(connlen > 0){
            // 获取http请求数据
            char buf[4096] = {0};
            char user_name[USER_LEN] = {0};
            char password[PASSWORD_LEN] = {0};

            int redlen = fread(buf,1,connlen,stdin);
            if(redlen == 0)
            {
                // 当没获取到数据则重新获取
                continue;
            }
            // 获取json的请求
            if(get_string_value(buf,"user_name",user_name) == -1 || get_string_value(buf,"password",password) == -1)
            {
                ret = -1;
                goto END;
            }

            // 查询账户是否存在
            ret = check_user_pwd(user_name,password);
            if(ret == 0)
            {
                set_token(user_name,token);
            }
        }else ret = -1;
END:
        if(ret == 0)       // 登录成功
        {
            out = respost_code("000","token",token);
        }
        else
        {             // 出错
            out = respost_code("404",NULL,NULL);
        } 

        if(out != NULL)
        {
            printf(out);
            free(out);
        }
    }
    return 0;
}

// gcc -o login_cgi login_cgi.c ../cfg.c ../cJSON.c ../base64.c ../redis_c_api.c ../mysql_c_aip.c -I ../../include/ -lmysqlclient -lfcgi -lcrypto -lhiredis

