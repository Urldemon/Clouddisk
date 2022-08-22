#ifndef AESKEY_H
#define AESKEY_H
#pragma execution_character_set("utf-8")
// 單例對象
// 负责加密解密的对象

#include <QByteArray>
#include "qaesencryption.h"

class AesKeyInstance
{
public:

    static AesKeyInstance* getSpyObj();
    static void destroy();

    QByteArray encode(QByteArray input8bit);
    QByteArray decode(QByteArray entext);

private:
    AesKeyInstance();

    class Garbo //设置为私有防止外界访问
            {
                public:
                    ~Garbo()//实际去析构new的单例对象
                    {
                        AesKeyInstance::destroy();
                    }
            };

     static Garbo garbo; //静态私有的嵌套类对象,防止被外界访问

    static AesKeyInstance* spyobj;

    static QAESEncryption* encryption;

    QByteArray hashkey;

    QByteArray hashive;

};

#endif // AESKEY_H
