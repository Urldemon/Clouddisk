CC = gcc
# 获取函数
COMMON = $(wildcard ./src/common/*.c)
COMMON_OBJ = $(patsubst ./src/common/%.c,./obj/%.o,$(COMMON))

CGI = $(wildcard ./src/cgi/*.c)
CGI_OBJ = $(patsubst ./src/cgi/%.c,./obj/%.o,$(CGI))

# 路劲对象
CGI_PATH = ./src/cgi
BIN_PATH = ./bin
OBJ_PATH = ./obj

#生成对象路径及名称
login_cgi = $(BIN_PATH)/login_cgi
uploadfile_cgi = $(BIN_PATH)/uploadfile_cgi
reup_cgi = $(BIN_PATH)/reup_cgi
ctl_cgi = $(BIN_PATH)/ctl_cgi
sharectl_cgi = $(BIN_PATH)/sharectl_cgi
gainfile_cgi = $(BIN_PATH)/gainfile_cgi
sharefile_cgi = $(BIN_PATH)/sharefile_cgi
md5_cgi = $(BIN_PATH)/md5_cgi

# 设置目标
target=$(login_cgi)		\
	   $(uploadfile_cgi) \
	   $(reup_cgi) \
	   $(ctl_cgi) \
	   $(sharectl_cgi) \
	   $(gainfile_cgi) \
	   $(sharefile_cgi) \
	   $(md5_cgi)
ALL:$(target)

# 应用路径
INCLUDE = -I ./include \
		  -I /usr/include/fastdfs/

# 编译所需的库
LIB = -lmysqlclient \
	  -lfdfsclient \
	  -lhiredis \
	  -lfcgi 	\
	  -lcrypto

# 生成对象的连接器
$(login_cgi):$(COMMON_OBJ) $(OBJ_PATH)/login_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(uploadfile_cgi):$(COMMON_OBJ) $(OBJ_PATH)/uploadfile_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(reup_cgi):$(COMMON_OBJ) $(OBJ_PATH)/reup_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(ctl_cgi):$(COMMON_OBJ) $(OBJ_PATH)/ctl_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(sharectl_cgi):$(COMMON_OBJ) $(OBJ_PATH)/sharectl_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(gainfile_cgi):$(COMMON_OBJ) $(OBJ_PATH)/gainfile_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(sharefile_cgi):$(COMMON_OBJ) $(OBJ_PATH)/sharefile_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)

$(md5_cgi):$(COMMON_OBJ) $(OBJ_PATH)/md5_cgi.o
	$(CC) -o $@ $^ $(INCLUDE) $(LIB)


# COMMON的生成规则
$(COMMON_OBJ):./obj/%.o:./src/common/%.c
	$(CC) -c $< -o $@ $(INCLUDE) $(LIB)

# CGI的生成规则
$(CGI_OBJ):./obj/%.o:./src/cgi/%.c
	$(CC) -c $< -o $@ $(INCLUDE) $(LIB)

clean:
	-rm -rf ./obj/*.o
.PHONY:clean ALL


