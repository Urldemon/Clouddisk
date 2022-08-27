#ifndef UPLOADTASK_H
#define UPLOADTASK_H

#include <QString>
#include <QFile>
#include "selfwidget/dataprogress.h"
#include "common.h"

struct UploadFileDate{
    QString filename;       // 文件名称
    QString path;           // 文件存储路径
    qint64 size;            // 文件大小
    QString md5;            // 文件md5码
    QFile *file;            // 文件指针
    bool isUpload;          // 是否已经在上传
    DataProgress *dp;       // 上传进度控件
};

class UpLoadTask
{
public:
    static UpLoadTask* getInstance();
    static void destroy();

    // 任务列表
    int appendUploadList(QString path);
    void clearList();
    bool isEmpty();
    bool isUpload();
    UploadFileDate* takeTask();
    void dealUploadTask();

private:
    UpLoadTask();

    //它的唯一工作就是在析构函数中删除Singleton的实例
    class Garbo
    {
    public:
        ~Garbo()
        {
            UpLoadTask::destroy();
        }
    };
    static Garbo garbo; //静态私有的嵌套类对象,防止被外界访问
private:
    static UpLoadTask *instance;

    Common m_common;

    QList<UploadFileDate*> list;        // 上传任务列表
};

#endif // UPLOADTASK_H
