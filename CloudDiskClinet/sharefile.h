#ifndef SHAREFILE_H
#define SHAREFILE_H
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QWidget>
#include <QMenu>
#include <QTimer>
#include <QNetworkAccessManager>
#include "common/common.h"


namespace Ui {
class ShareFile;
}

class ShareFile : public QWidget
{
    Q_OBJECT

public:
    explicit ShareFile(QWidget *parent = nullptr);
    ~ShareFile();

    // 初始化ListWidget属性
    void initListWidget();
    void addActionMenu();

    // ======文件显示=======
    void refreshFileItems();                      // 文件item展示
    void clearshareFileList();                    // 清空文件列表
    void clearItem();                             // 清除listWidget上的item对象

    void refreshFiles();                          // 显示共享的文件列表
    void getUserFilesList(Display cmd = Normal);  // 获取共享文件列表
    void getFileJsonInfo(QByteArray data);       // 将json数据加入列表中

    // ======json数据处理====
    QString getCountStatus(QByteArray json);      // 获取count
    QByteArray setFilesListJson(QString user,QString token,int start,int count);
    QByteArray setDealFileJson(QString user,QString token,QString filename,QString md5);        // 整合json包

    // =====文本右键操作=======
    void dealSelectdFile(QString cmd);

    void downFile(FileInfo *info);
    void saveFile(FileInfo *info);
    void getFileProperty(FileInfo *info);

    // ==== 下载相关=====
    void downloadFilesAction();
    void dealFilePv(QString md5,QString filename);

    // =========定时器==============
    void checkTaskList();

signals:
    void loginAgainSignal();
    void gotoTransfer(TransferStatus status);
private:
    Ui::ShareFile *ui;

    Common m_common;
    QNetworkAccessManager *m_manger;

    QList<FileInfo *> m_shareFileList;              //文件列表

    long m_shareFilesCount;                          // 用户列表数量

    int m_start;
    int m_count;

private:

    QMenu *m_menuItem;                              // item菜单
    QAction *m_downloadAction;                      // 下载
    QAction *m_saveAction;                          // 转存文件
    QAction *m_propertyAction;                      // 属性

    QMenu *m_menuEmpty;                             // 空位置菜单
    QAction *m_refreshAction;                       // 刷新

    void rightMenu(const QPoint &pos);
private:
    QTimer m_downloadTimer;         //定时检查下载队列是否有任务需要下载
};

#endif // SHAREFILE_H
