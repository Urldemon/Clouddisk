#ifndef LOGININFOINSTANCE_H
#define LOGININFOINSTANCE_H
#pragma execution_character_set("utf-8")
#include <QString>

// 单例模式
// 保存该当前登录用户的信息及服务器信息
class LoginInfoInstance
{
public:
    // 单例外部访问接口
    static LoginInfoInstance* getInstance();        // 获取单例对象
    static void destroy();                          // 释放单例对象

    void setLoginInfo(QString user,QString ip,QString port,QString token="");
    // 外部访问接口
    QString getUser() const;
    QString getToken() const;
    QString getIp() const;
    QString getPort() const;

private:
    LoginInfoInstance();
    class Garbo //设置为私有防止外界访问
    {
        public:
            ~Garbo()//实际去析构new的单例对象
            {
                LoginInfoInstance::destroy();
            }
    };
     static Garbo garbo; //静态私有的嵌套类对象,防止被外界访问

private:

    QString user;       // 当前登录的用户
    QString token;      // 登录的token
    QString ip;         // 服务器ip
    QString port;       // 服务器端口号
    // 静态数据成员
    static LoginInfoInstance* instance;
};

#endif // LOGININFOINSTANCE_H
