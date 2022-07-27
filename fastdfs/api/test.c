#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fdfs_c_api.h"

#define CONF_PATH "../fastdfsconf/client.conf"
int main(int argc,char *argv[]){
    if(argc < 2){
        printf("输出参数不对\n");
        exit(-1);
    }
    char file_id[1024] = {0};
    printf("%s\n",argv[1]);
    fdfs_upload_file(CONF_PATH,argv[1],file_id);
    printf("%s\n",file_id);
}

// gcc -o test test.c fdfs_c_api.c -I /usr/include/fastdfs -lfdfsclient
