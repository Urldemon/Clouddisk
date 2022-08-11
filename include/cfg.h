#ifndef _CFG_H_
#define _CFG_H_

# define CONF_PATH "/home/hang/github/PanServer/server_conf.json"

int get_conf_value(const char *jsonfile,const char *obj,const char *key,char *value);
int get_mysql_data(char *user,char *password,char *database);
int get_conf_inet_value(char *obj,char *ip,char *port);
int get_string_value(const char *str,const char *obj,char *value);

char *respost_code(char *token,char *ret);
#endif