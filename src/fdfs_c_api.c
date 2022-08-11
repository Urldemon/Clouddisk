#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "fdfs_client.h"
#include "fdfs_c_api.h"

int fdfs_upload_file(const char *conf_filename,const char *local_filename,char *filer_id)
{
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	ConnectionInfo *pTrackerServer;
	int result;
	int store_path_index;
	ConnectionInfo storageServer;
	
    // 初始化
	if ((result=fdfs_client_init(conf_filename)) != 0)
	{
		return result;
	}
    // 从配置文件中获取tracker连接信息
	pTrackerServer = tracker_get_connection();
	if (pTrackerServer == NULL)
	{
		fdfs_client_destroy();
		return errno != 0 ? errno : ECONNREFUSED;
	}

	*group_name = '\0';
    // 获取存储节点信息
	if ((result=tracker_query_storage_store(pTrackerServer, \
	                &storageServer, group_name, &store_path_index)) != 0)
	{
		fdfs_client_destroy();
		fprintf(stderr, "tracker_query_storage fail, " \
			"error no: %d, error info: %s\n", \
			result, STRERROR(result));
		return result;
	}
    // 文件上传
	result = storage_upload_by_filename1(pTrackerServer, \
			&storageServer, store_path_index, \
			local_filename, NULL, \
			NULL, 0, group_name, filer_id);
	if (result != 0)
	{
		fprintf(stderr, "upload file fail, " \
			"error no: %d, error info: %s\n", \
			result, STRERROR(result));
	}

	tracker_close_connection_ex(pTrackerServer, true);
	fdfs_client_destroy();

	return result;
}

void my_fdfs_upload_file(const char *conf_filename,const char *local_filename,char *filer_id,int size){
    // 1. 创建匿名管道
    int fd[2];
    int ret = pipe(fd);
    if(ret == -1){
        perror("pipe error");

        exit(-1);
    }
    // 2.创建子进程
    pid_t pid = fork();
    // 子进程
    if(pid == 0){
        // 3.标准输出重定向
        dup2(fd[1],STDOUT_FILENO);
        // 4.关闭读端
        close(fd[0]);
        // 5.执行execlp
        execlp("fdfs_upload_file","xxx",conf_filename,local_filename,NULL);
        perror("execlp error");
    }else{  //父进程
        // 6.关闭写端
        close(fd[1]);
        // 7.读取数据
        read(fd[0],filer_id,size);
        // 回收子进程
        wait(NULL);
    }
}

int fdfs_delete_file(const char *conf_filename,const char *file_id)
{
	ConnectionInfo *pTrackerServer;
	int result;
	
	if ((result=fdfs_client_init(conf_filename)) != 0)
	{
		return result;
	}

	pTrackerServer = tracker_get_connection();
	if (pTrackerServer == NULL)
	{
		fdfs_client_destroy();
		return errno != 0 ? errno : ECONNREFUSED;
	}

	if ((result=storage_delete_file1(pTrackerServer, NULL, file_id)) != 0)
	{
		printf("delete file fail, " \
			"error no: %d, error info: %s\n", \
			result, STRERROR(result));
	}

	tracker_close_connection_ex(pTrackerServer, true);
	fdfs_client_destroy();
	return result;
}
