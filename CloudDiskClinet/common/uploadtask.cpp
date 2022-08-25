#include "uploadtask.h"
#include <QDebug>
#include <QFileInfo>
#include <QVBoxLayout>
#include "uploadlayout.h"

UpLoadTask* UpLoadTask::instance = new UpLoadTask;

UpLoadTask::Garbo UpLoadTask::garbo;

UpLoadTask *UpLoadTask::getInstance()
{
    return instance;
}

void UpLoadTask::destroy()
{
    if(UpLoadTask::instance != nullptr)
    {
        UpLoadTask::instance->clearList();

        delete instance;
        instance = nullptr;
        cout << "instance is delete";
    }
}

int UpLoadTask::appendUploadList(QString path)
{
    for(auto val : list)
    {
        if(val->path == path)
        {
            cout << QFileInfo(path).fileName() << "已经在上传列表中了" ;
            return -2;                  // 文件存在任务列表
        }
    }

    QFile *file = new QFile(path);
    if(file->open(QIODevice::ReadOnly) == false)
    {
        cout << "file open error";
        delete file;
        file = nullptr;
        return -3;
    }

    // 获取文件属性
    QFileInfo info(path);

    // 创建结点
    /*
    QString filename;       // 文件名称
    QString path;           // 文件存储路径
    qint64 size;            // 文件大小
    QString md5;            // 文件md5码
    QFile *file;            // 文件指针
    bool isUpload;          // 是否已经在上传
    DataProgress *dp;       // 上传进度控件
    */
    UploadFileDate *tmp = new UploadFileDate;
    tmp->filename = info.fileName();
    tmp->path = path;                               // 文件路径
    tmp->size = info.size();                        // 获取文件大小
    tmp->md5 = m_common.getFileMd5(path);           // 文件生成md5
    tmp->file = file;                               // 文件指针 在任务处理时释放
    tmp->isUpload = false;                          // 当前文件没有被上传

    DataProgress *p = new DataProgress;             // 创建任务对应的进度条控件
    p->setFileName(tmp->filename);                  // 设置文件名并初始化
    tmp->dp = p;                                    // 赋值

    // 获取布局
    UpLoadLayout *pUpload = UpLoadLayout::getInstance();
    if(pUpload == nullptr)
    {
        cout << "UploadTask::getInstance() == NULL";
        return -4;
    }

    QVBoxLayout *layout = (QVBoxLayout*)pUpload->getUpLoadLayout();
    // 添加到布局，最后一个是弹簧，插入到当黄上边
    layout->insertWidget(layout->count()-1, p);

    cout << tmp->filename.toUtf8().data() << "已经放在上传列表";

    // 在末尾插入节点
    list.push_back(tmp);

    return 0;
}

void UpLoadTask::clearList()
{
    for(auto val : list)
    {
        delete val;
    }
}

// 判断任务队列是否有数据
bool UpLoadTask::isEmpty()
{
    return list.isEmpty();
}

// 判断是否有文件正在上传
bool UpLoadTask::isUpload()
{
    for(auto val : list)
    {
        if(val->isUpload == true)
        return true;
    }
    return false;
}

// 获取最头的任务
UploadFileDate *UpLoadTask::takeTask()
{

    if(isEmpty() == true)return nullptr;
    // 取出第一个任务
    UploadFileDate *tmp = list.front();
    tmp->isUpload = true;

    return tmp;
}

// 删除上传完成的任务
void UpLoadTask::dealUploadTask()
{
    UploadFileDate *val = list.front();
    if(val->isUpload == true)
    {
        // 移除文件，因为上传成功了
        // 获取布局
        UpLoadLayout *pUpload = UpLoadLayout::getInstance();
        QLayout *layout = pUpload->getUpLoadLayout();
        layout->removeWidget(val->dp);

        // 关闭打开的文件执政
        val->file->close();

        // 释放组件对象
        delete val->dp;
            // 释放数组对象
        delete val;

        // 删除当前头指针
        list.pop_front();
    }
}

UpLoadTask::UpLoadTask()
{

}
