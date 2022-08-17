#ifndef _CFG_H_
#define _CFG_H_

# define CONF_PATH "/home/hang/github/PanServer/server/server_conf.json"

int get_conf_value(const char *jsonfile,const char *obj,const char *key,char *value);
int get_mysql_data(char *user,char *password,char *database);
int get_conf_inet_value(char *obj,char *ip,char *port);
int get_string_value(const char *str,const char *obj,char *value);

int get_query_data(const char *query,const char *key,char *cmd);
char *respost_code(const char *ret,const char *obj_name,const char *value);
int verify_token(const char *user_name,const char *token);
#endif
