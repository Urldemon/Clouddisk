#ifndef FDSF_C_API
#define FDSF_C_API
int fdfs_upload_file(const char *conf_filename, const char *local_filename,char *file_id);
void my_fdfs_upload_file(const char *conf_filename,const char *local_filename,char *file_id,int size);
int fdfs_delete_file(const char *conf_filename,const char *file_id);
#endif
