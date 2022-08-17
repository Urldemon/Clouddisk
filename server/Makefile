COMMON = $(wildcard ./src/common/*.c)
COMMON_OBJ = $(patsubst ./src/common/%.c,./obj/%.o,$(COMMON))

sgi = $(wildcard ./src/cgi/*.c)
z = $(patsubst ./src/cgi/%.c,./obj/%.o,$(sgi))

CGI_PATH = ./src/cgi
BIN_PATH = ./bin

login_cgi = $(BIN_PATH)/login_cgi
uploadfile_cgi = $(BIN_PATH)/uploadfile_cgi
reup_cgi = $(BIN_PATH)/reup_cgi
ctl_cgi = $(BIN_PATH)/ctl_cgi
sharectl_cgi = $(BIN_PATH)/sharectl_cgi
gainfile_cgi = $(BIN_PATH)/gainfile_cgi
sharefile_cgi = $(BIN_PATH)/sharefile_cgi



target=$(login_cgi)		\
	   $(uploadfile_cgi) \
	   $(reup_cgi) \
	   $(ctl_cgi) \
	   $(sharectl_cgi) \
	   $(gainfile_cgi) \
	   $(sharefile_cgi) 
ALL:$(target)

INCLUDE = -I ./include \
		  -I /usr/include/fastdfs/

LIB = -lmysqlclient \
	  -lfdfsclient \
	  -lhiredis \
	  -lfcgi 	\
	  -lcrypto

$(login_cgi):$(COMMON_OBJ) $(CGI_PATH)/login_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)
$(uploadfile_cgi):$(COMMON_OBJ) $(CGI_PATH)/uploadfile_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)
$(reup_cgi):$(COMMON_OBJ) $(CGI_PATH)/reup_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)
$(ctl_cgi):$(COMMON_OBJ) $(CGI_PATH)/ctl_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)
$(sharectl_cgi):$(COMMON_OBJ) $(CGI_PATH)/sharectl_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)
$(gainfile_cgi):$(COMMON_OBJ) $(CGI_PATH)/gainfile_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)
$(sharefile_cgi):$(COMMON_OBJ) $(CGI_PATH)/sharefile_cgi.o
	gcc -o $@ $^ $(INCLUDE) $(LIB)

$(COMMON_OBJ):./obj/%.o:./src/common/%.c
	gcc -c $< -o $@ $(INCLUDE) $(LIB)

%.o:%.c
	gcc -c $< -o $@ $(INCLUDE) $(LIB)

clean:
	-rm -rf *.o $(CGI_PATH)/*.o ./obj/*.o
.PHONY:clean ALL

