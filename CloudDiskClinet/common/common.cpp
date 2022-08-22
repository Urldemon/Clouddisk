#include "common.h"
#include "common/common.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopWidget>
#include <QApplication>
#include <QFile>
#pragma execution_character_set("utf-8")

// 单例初始化（饿汉式）
QNetworkAccessManager* Common::m_netManager = new QNetworkAccessManager;



Common::Common(QObject* parent)
{
    Q_UNUSED(parent)
    // 初始化对象
    m_aeskey = AesKeyInstance::getSpyObj();
}

Common::~Common()
{

}
QNetworkAccessManager* Common::getNetManager()
{
    // 该对象已近创建直接使用
    return m_netManager;
}


QString Common::getCode(QByteArray array)
{
    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(array,&error);

    if(error.error != QJsonParseError::NoError)
    {
        qDebug() << "[ " << __FILE__ << ":"  << __LINE__ << " ] " << "err = " << error.errorString();
        return "";
    }

    if(doc.isNull() == true || doc.isEmpty() == true )
    {
        qDebug() << "[ " << __FILE__ << ":"  << __LINE__ << " ] " << "doc.isNull() || doc.isEmpty()";
        return "";
    }

    if(doc.isObject())
    {
        // 将doc的数据转化为json对象
        QJsonObject obj = doc.object();
        // 获取recode对象数据
        return obj.value("retcode").toString();
    }
    return "";
}


QString Common::getConfvalue(QString title, QString key, QString path)
{
    QFile file(path);

    // 只读的方式打开
    if( file.open(QIODevice::ReadOnly) == false)
    {
        qDebug() << "[ " << __FILE__ << ":"  << __LINE__ << " ] " << "file open error";
        return "";
    }

    QByteArray array = file.readAll();          // 读取所有数据
    file.close();                               // 关闭文件

    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(array,&error);
    if(error.error != QJsonParseError::NoError)
    {
        qDebug() << "[ " << __FILE__ << ":"  << __LINE__ << " ] " << "err = " << error.errorString();
        return "";
    }
    if(doc.isNull() == true || doc.isEmpty() == true )
    {
        qDebug() << "[ " << __FILE__ << ":"  << __LINE__ << " ] " << "doc.isNull() || doc.isEmpty()";
        return "";
    }
    if(doc.isObject())
    {
        // 获取整体对象
        QJsonObject obj = doc.object();
        // 获取指定键的对象
        QJsonObject tmp = obj.value(title).toObject();
        // 输出数值
        return  tmp.value(key).toString();
    }
    return "";
}


void Common::writeWebInfo(QString ip, QString port, QString path)
{
    // server配置信息
    QMap<QString,QVariant> server;
    server.insert("ip",ip);
    server.insert("port",port);

    // 重修数据
    QMap<QString,QVariant> typ_path;
    typ_path.insert("path",path);

    // 将原有数据导出在写入
    QString user = getConfvalue("login","user");
    QString pwd = getConfvalue("login","pwd");
    QString remember = getConfvalue("login","remember");

    QMap<QString,QVariant> login;
    login.insert("user",user);
    login.insert("pwd",pwd);
    login.insert("remember",remember);

    // 将数据整合
    QMap<QString,QVariant> json;
    json.insert("server",server);
    json.insert("login",login);
    json.insert("type_path",typ_path);


    // 将所有数据写入json对象中
    QJsonDocument doc = QJsonDocument::fromVariant(json);
    if(doc.isNull() == true)
    {
        qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]" << "QJsonDocument::fromVariant(json) error";
        return ;
    }

    QFile file(path);
    if(file.open(QIODevice::WriteOnly) == false)
    {
        qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]" << "openfile error";
        return ;
    }

    qint64 line = file.write(doc.toJson());
    if(line < 0)
    {
        qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]" << "write error";
        return ;
    }
    file.close();
}

void Common::writeSgininInfo(QString user, QString pwd, bool isremeber, QString path)
{
    // 获取原有数据并重新写入QMap
    QMap<QString,QVariant> server;
    server.insert("ip",getConfvalue("server","ip"));
    server.insert("port",getConfvalue("server","port"));

    QMap<QString,QVariant> type_path;
    type_path.insert("path",getConfvalue("type_path","path"));

    QMap<QString,QVariant> login;
    // 对用户名进行加密
    QByteArray useren = m_aeskey->encode(user.toLocal8Bit()).toBase64();
    // 对密码进行加密
    QByteArray pwden = m_aeskey->encode(pwd.toLocal8Bit()).toBase64();
    // 获取isremeber
    if(isremeber == true)
        login.insert("remember","yes");
    else
        login.insert("remember","no");

    login.insert("user",useren);
    login.insert("pwd",pwden);

    // 写入json
    QMap<QString,QVariant> json;
    json.insert("login",login);
    json.insert("server",server);
    json.insert("type_path",type_path);

    // 转化成json格式
    QJsonDocument doc = QJsonDocument::fromVariant(json);
    if(doc.isNull() == true || doc.isEmpty() == true)
    {
        qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]" << " QJsonDocument::fromVariant(json) err";
    }

    // 打开并写入文件
    QFile file(path);
    if(file.open(QIODevice::WriteOnly) == false)
    {
        qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]" << " file.open err";
    }

    file.write(doc.toJson());
    file.close();
}

QString Common::getStrSha256(QString str)
{
    QByteArray array;
    array = QCryptographicHash::hash(str.toLocal8Bit(),QCryptographicHash::Sha256);

    return array.toHex();
}

void Common::moveToCenter(QWidget *tmp)
{
    QDesktopWidget* desktop = QApplication::desktop();   // 获取屏幕中央

    tmp->move((desktop->width() - tmp->width())/2,(desktop->height() - tmp->height())/2);
}

void Common::getFileTypeList()
{

}
