#include "logininfoinstance.h"
#include <QDebug>
#pragma execution_character_set("utf-8")

LoginInfoInstance* LoginInfoInstance::instance = new LoginInfoInstance;

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
        qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]" << "instance is delete";
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
