#ifndef _BASE64_H_
#define _BASE64_H_

const char *base64_encode(const unsigned char *bindata,int binlength,char *base64);

int base64_decode(const char *base64,unsigned char *bindata);

#endif 
