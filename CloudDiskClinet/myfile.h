#ifndef MYFILE_H
#define MYFILE_H
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QWidget>
#include <QTimer>
#include "common/common.h"
#include "common/uploadtask.h"
#include "selfwidget/mymenu.h"

namespace Ui {
class MyFile;
}



class MyFile : public QWidget
{
    Q_OBJECT

public:
    explicit MyFile(QWidget *parent = nullptr);
    ~MyFile();

    // 初始化ListWidge文件列表
    void initListWidget();
    // 添加右键菜单
    void addActionMenu();

    // ======上传文件=======
    void addUploadFiles();                                                                      // 添加上传文件
    void uploadFileAction();                                                                    // 执行上传操作，先判断MD5快传，无快传则再执行正常上传
    void uploadFile(UploadFileDate *upload);                                                    // 正常文件上传步骤

    // ========显示用户信息===========
    void refreshFileItems();                                                                    // 文件item展示
    void addUploadItem(QString iconPath=":/image/upload.png",QString name="上传文件");           // 添加上传文件item
    void clearFileList();                                                                       // 清空文件列表
    void clearItems();                                                                          // 清空所有item项目

    void refreshFile(Display cmd = Normal);                                                     // 显示用户的文件列表
    void getUserFilesList(Display cmd = Normal);                                                // 获取用户文件列表
    void getFileJsonInfo(QByteArray data);                                                      // 将获取到的数据写入文件列表中

    // ========文件右键操作=========
    void processSelectedFiles(QString cmd);

    void delFile(FileInfo *info);                                                               // 删除某个文件
    void downFile(FileInfo *info);                                                              // 下载文件
    void getFileProperty(FileInfo *info);                                                       // 获取属性
    void shareFile(FileInfo *info);                                                             // 分享文件
    void dealshareFile(FileInfo *info);                                                         // 取消分享

    // ========下载相关操作 ========
    void downloadFilesAction();                                                                 // 执行下载操作
    void dealFilePv(QString md5,QString filename);                                              // 更新下载标识

    // ========生成json方法========
    QByteArray setDealFileJson(QString user,QString token,QString filename,QString md5);        // 整合json包
    QByteArray setMd5Json(QString user,QString token,QString md5,QString filename);             // 生成上传文件的json格式
    QByteArray setFilesListJson(QString user,QString token,int start,int count);
    QString getCountStatus(QByteArray json);                                                    // 获取服务器返回信息

    // =========定时器==============
    void checkTaskList();

signals:
    void gotoTransfer(TransferStatus status);
    void loginAgainSignal();

private:
    Ui::MyFile *ui;

    Common m_common;
    QNetworkAccessManager *m_manager;

    // 文件列表
    QList<FileInfo*> m_fileList;

    // 用户列表数量
    long m_userFilesCount;

    int m_start;
    int m_count;
private:                // 右键操作


    // 菜单一
    QMenu *m_menu;                          // 菜单
    QAction *m_downloadAction;              // 下载操作
    QAction *m_shareAction;                 // 分享操作
    QAction *m_cancelAction;
    QAction *m_delAction;                   // 删除操作
    QAction *m_propertyAction;              // 属性

    // 菜单二
    QMenu   *m_menuEmpty;                   // 菜单2
    QAction *m_pvAscendingAction;           // 按下载量升序
    QAction *m_pvDescendingAction;          // 按下载量降序
    QAction *m_refreshAction;               // 刷新
    QAction *m_uploadAction;                // 上传

    void rightMenu(const QPoint &pos);
private:
    //定时器
    QTimer m_uploadFileTimer;       //定时检查上传队列是否有任务需要上传
    QTimer m_downloadTimer;         //定时检查下载队列是否有任务需要下载
};

#endif // MYFILE_H
