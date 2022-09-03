#include "logininfoinstance.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include "common.h"
#include <QDebug>


LoginInfoInstance* LoginInfoInstance::instance = new LoginInfoInstance;

//static类的析构函数在main()退出后调用
LoginInfoInstance::Garbo LoginInfoInstance::garbo; //静态数据成员，类中声明，类外定义

LoginInfoInstance *LoginInfoInstance::getInstance()
{
    return instance;
}

void LoginInfoInstance::destroy()
{
    if(LoginInfoInstance::instance != nullptr)
    {
        delete LoginInfoInstance::instance;
        LoginInfoInstance::instance = nullptr;
        cout << "instance is delete";
    }
}

void LoginInfoInstance::setLoginInfo(QString user, QString ip, QString port, QString token)
{
    this->user = user;
    this->ip = ip;
    this->port = port;
    this->token = token;
}

QString LoginInfoInstance::getUser() const
{
    return user;
}

QString LoginInfoInstance::getToken() const
{
    return token;
}

QString LoginInfoInstance::getIp() const
{
    return ip;
}

QString LoginInfoInstance::getPort() const
{
    return port;
}

LoginInfoInstance::LoginInfoInstance()
{

}
