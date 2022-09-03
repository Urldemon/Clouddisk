#include "common.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopWidget>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QMessageBox>

#pragma execution_character_set("utf-8")

// 单例初始化（饿汉式）
QNetworkAccessManager* Common::m_netManager = new QNetworkAccessManager;

// 类外初始化
QStringList Common::m_typeList = QStringList();

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
        cout << "err = " << error.errorString();
        return "";
    }

    if(doc.isNull() == true || doc.isEmpty() == true )
    {
        cout << "doc.isNull() || doc.isEmpty()";
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
        cout << "file open error";
        return "";
    }

    QByteArray array = file.readAll();          // 读取所有数据
    file.close();                               // 关闭文件

    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(array,&error);
    if(error.error != QJsonParseError::NoError)
    {
        cout << "err = " << error.errorString();
        return "";
    }
    if(doc.isNull() == true || doc.isEmpty() == true )
    {
        cout << "doc.isNull() || doc.isEmpty()";
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

QByteArray Common::setGetCountjson(QString user, QString token)
{
    QMap<QString,QVariant> tmp;
    tmp.insert("user_name",user);
    tmp.insert("token",token);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if(jsonDocument.isNull())
    {
        cout << "jsonDocument.isNull()";
        return nullptr;
    }
    return jsonDocument.toJson();
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
       cout << "QJsonDocument::fromVariant(json) error";
        return ;
    }

    QFile file(path);
    if(file.open(QIODevice::WriteOnly) == false)
    {
       cout << "openfile error";
        return ;
    }

    qint64 line = file.write(doc.toJson());
    if(line < 0)
    {
       cout << "write error";
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
       cout << " QJsonDocument::fromVariant(json) err";
    }

    // 打开并写入文件
    QFile file(path);
    if(file.open(QIODevice::WriteOnly) == false)
    {
       cout << " file.open err";
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

QString Common::getStrShaMd5(QString str)
{
    QByteArray array;
    array = QCryptographicHash::hash(str.toLocal8Bit(),QCryptographicHash::Md5);
    return array.toHex();
}

QString Common::getFileMd5(QString path)
{
    QFile file(path);
    if(file.open(QIODevice::ReadOnly) == false)
    {
        cout << "file open error";
        return "";
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    qint64 totaBytes = 0;                   // 记录文件大小
    quint64 bytesWritten = 0;               // 写入后所剩数据

    quint64 bytesToWrite = 0;               // 记录写入的数据大小

    quint64 loadSize = 1024 * 4;            // 每次写入数据大小

    QByteArray buf;

    totaBytes = file.size();
    bytesToWrite = totaBytes;

    while(1)
    {
        if(bytesToWrite > 0)
        {
            buf = file.read(qMin(bytesToWrite,loadSize));
            hash.addData(buf);
            bytesToWrite -= buf.size();             // 记录剩下多少
            bytesWritten += buf.size();             // 记录写了多少
            buf.resize(0);                          // 清空数据
        }
        else break;

        if(bytesWritten == totaBytes)break;             // 当写入了数据等于数据大小
    }

    file.close();

    QByteArray md5 = hash.result();                     // 加密成md5结构

    return md5.toHex();                                 // 转化成16进制
}

QString Common::getBoundary()
{
    // 设置随机数种子
    qsrand(time(NULL));
    QString tmp;

    // 48~122 '0'~'A'~'z';
    for(int i = 0;i < 16;++i)
    {
        tmp[i] = qrand() % (122 - 48) + 48;
    }

    return QString("------WebKitFormBoundary%1").arg(tmp);
}

void Common::moveToCenter(QWidget *tmp)
{
    QDesktopWidget* desktop = QApplication::desktop();   // 获取屏幕中央

    tmp->move((desktop->width() - tmp->width())/2,(desktop->height() - tmp->height())/2);
}

void Common::getFileTypeList()
{
    QDir dir(FILETYPEDIR);
    if(dir.exists() == false)                   // 不存在则创建
    {
        dir.mkpath(FILETYPEDIR);
        cout << FILETYPEDIR << "创建成功！！！";
    }

    /*
     * QDir的一些用法：
        QDir::Dirs      列出目录；
        QDir::AllDirs   列出所有目录，不对目录名进行过滤；
        QDir::Files     列出文件；
        QDir::Drives    列出逻辑驱动器名称，该枚举变量在Linux/Unix中将被忽略；
        QDir::NoSymLinks        不列出符号链接；
        QDir::NoDotAndDotDot    不列出文件系统中的特殊文件.及..；
        QDir::NoDot             不列出.文件，即指向当前目录的软链接
        QDir::NoDotDot          不列出..文件；
        QDir::AllEntries        其值为Dirs | Files | Drives，列出目录、文件、驱动器及软链接等所有文件；
        QDir::Readable      列出当前应用有读权限的文件或目录；
        QDir::Writable      列出当前应用有写权限的文件或目录；
        QDir::Executable    列出当前应用有执行权限的文件或目录；
        Readable、Writable及Executable均需要和Dirs或Files枚举值联合使用；
        QDir::Modified      列出已被修改的文件，该值在Linux/Unix系统中将被忽略；
        QDir::Hidden        列出隐藏文件；
        QDir::System        列出系统文件；
        QDir::CaseSensitive 设定过滤器为大小写敏感。
    */
    dir.setFilter(QDir::Files| QDir::NoDot |  QDir::NoDotDot | QDir::NoSymLinks);       // 过滤获取的文件
    dir.setSorting(QDir::Size | QDir::Reversed);                                        // 排序规则‘

    QFileInfoList list = dir.entryInfoList();                                           // 将其转化为list对象

    for(auto val : list)
    {
        m_typeList.append(val.fileName());                                              // 将文件名传入
    }

}

QString Common::getFileType(QString type)
{
    if(m_typeList.contains(type) == true)
    {
        return QString(FILETYPEDIR) + "/" + type;
    }

    return QString(FILETYPEDIR) + "/other.png";
}

void Common::writeRecord(QString user, QString filename, QString code, QString path)
{
    // 打开文件目录
    QDir dir(path);

    // 目录不存在
    if(dir.exists() == false)
    {
        QString str = "";
        // 目录创建
        if(dir.mkpath(path) == true)
            str += "目录创建成功";
        else
            str += "目录创建失败";

        cout << str;
    }

    QString filepath = path + user;             // 记录每个账户的上传下载信息文件的路径

    // 如果文件存在，先读取文件的原本来内容
    QFile file(filepath);

    if(file.exists())           // 判断文件是否存在
        file.open(QIODevice::Append);               // 文件数据写入末尾
    else
        file.open(QIODevice::WriteOnly);            // 打开文件并重新撰写

    // 写入数据准备
    QByteArray array;

    QDateTime time = QDateTime::currentDateTime();      // 获取系统现在时间
    QString timeStr = time.toString("yyyy-MM-dd hh:mm:ss ddd");     //  转换成时间格式

    /*
       秒传文件：
            文件不在存储区 {"code":"110"}
            文件已存在：  {"code":"111"}
            秒传成功：    {"code":"112"}

        上传文件：
            成功：{"code":"113"}
            失败：{"code":"114"}
        下载文件：
            成功：{"code":"115"}
            失败：{"code":"116"}
    */

    QString actionStr;                  // 记录状态

    if(code == "111")
        actionStr = "上传失败，文件已存在";
    else if(code == "112")
        actionStr = "秒传成功";
    else if(code == "113")
        actionStr = "上传成功";
    else if(code == "114")
        actionStr = "上传失败";
    else if(code == "115")
        actionStr = "下载成功";
    else if(code == "116")
        actionStr = "下载失败";
    else
    {
        file.close();
        return ;
    }

    QString str = QString("%1 [%2] :filename:[%3]\r\n").arg(timeStr).arg(actionStr).arg(filename);

    file.write(str.toLocal8Bit());

    file.close();
}
