#include "aeskeyinstance.h"
#include "common.h"
#include <QDebug>
#include <QCryptographicHash>
#pragma execution_character_set("utf-8")

#define HASHKEY "1234567887654321"
#define HASHIVE "0000000000000000"

//static类的析构函数在main()退出后调用
AesKeyInstance::Garbo AesKeyInstance::garbo; //静态数据成员，类中声明，类外定义

AesKeyInstance* AesKeyInstance::spyobj = new AesKeyInstance;


QAESEncryption* AesKeyInstance::encryption = new QAESEncryption(QAESEncryption::AES_256,QAESEncryption::CBC);

AesKeyInstance::AesKeyInstance()
{
    hashkey = QCryptographicHash::hash(QString(HASHKEY).toLocal8Bit(),QCryptographicHash::Sha256);
    hashive = QCryptographicHash::hash(QString(HASHIVE).toLocal8Bit(),QCryptographicHash::Md5);
}

AesKeyInstance *AesKeyInstance::getSpyObj()
{
    return spyobj;
}

void AesKeyInstance::destroy()
{
    if(AesKeyInstance::encryption != nullptr)
    {
        delete AesKeyInstance::encryption;
        AesKeyInstance::encryption = nullptr;
    }
    if(AesKeyInstance::spyobj != nullptr)
    {
        delete AesKeyInstance::spyobj;
        AesKeyInstance::spyobj = nullptr;
    }
    cout << "spyobj delete";
}

QByteArray AesKeyInstance::encode(QByteArray input8bit)
{
    return encryption->encode(input8bit,hashkey,hashive);   // 加密
}

QByteArray AesKeyInstance::decode(QByteArray entext)
{
    QByteArray decodeText = encryption->decode(entext,hashkey,hashive); // 解密
    return encryption->removePadding(decodeText);       // 将解密后的空白去除
}
