#ifndef COMMON2_H
#define COMMON2_H
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QCoreApplication>
#include "aeskeyinstance.h"


#define CONFFILE        "conf/cfg.json"       // 存放配置信息的目录
#define FILETYPEDIR     "conf/fileType/"                                     // 存放文件类型图片目录
#define RECORDDIR       "conf/record/"                                      // 用户文件上传下载记录

#define cout  qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]"


#define USER_REG        "^[a-zA-Z\\d_@#-\*]\{3,16\}$"
#define PASSWD_REG      "^[a-zA-Z\\d_@#-\*]\{6,18\}$"
#define PHONE_REG       "1\\d\{10\}"
#define EMAIL_REG       "^[a-zA-Z\\d\._-]\+@[a-zA-Z\\d_\.-]\+(\.[a-zA-Z0-9_-]\+)+$"
#define IP_REG          "((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)"
#define PORT_REG        "^[1-9]$|(^[1-9][0-9]$)|(^[1-9][0-9][0-9]$)|(^[1-9][0-9][0-9][0-9]$)|(^[1-6][0-5][0-5][0-3][0-5]$)"

// 传输状态
enum TransferStatus{Download, Uplaod, Recod};

// 文件信息
struct FileInfo
{
    QString md5;            // 文件md5码
    QString fileName;       // 文件名字
    QString user;           // 用户
    QString createTime;     // 上传时间
    QString url;            // url
    QString type;           // 文件类型
    qint64 size;            // 文件大小
    int shareStatus;        // 是否共享, 1共享， 0不共享
    int pv;                 // 下载量
    QListWidgetItem *item;  // list widget 的item
};

class Common:public QObject
{
    Q_OBJECT
public:
    Common(QObject* parent = 0);
    ~Common();

    QString getCode(QByteArray array);
    QString getConfvalue(QString title,QString key,QString path= CONFFILE);

    QByteArray setGetCountjson(QString user,QString token);                                       // 生成认证登录信息

    void writeWebInfo(QString ip,QString port,QString path= CONFFILE);
    void writeSgininInfo(QString user,QString pwd,bool isremeber,QString path= CONFFILE);

    void writeRecord(QString user, QString filename, QString code, QString path = RECORDDIR);  // 将传输数据写入磁盘

    QString getStrSha256(QString str);                                  // 对数进行sha256加密

    QString getFileMd5(QString path);                                   // 生成文件md5值

    QString getBoundary();                                              // 生成文件传输所用边界

    void moveToCenter(QWidget *tmp);                                    // 将软件剧中

    void getFileTypeList();                                             // 初始化图片数据



public:
    static QNetworkAccessManager* getNetManager();

    static AesKeyInstance* getAesSpy();
private:

    // http类
    static QNetworkAccessManager *m_netManager;

    // 加密管理类
    AesKeyInstance *m_aeskey;
};

#endif // COMMON2_H
