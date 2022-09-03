#ifndef RANKING_H
#define RANKING_H
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QWidget>
#include <common/common.h>
#include <QNetworkAccessManager>
#include "common/logininfoinstance.h"

namespace Ui {
class Ranking;
}
// 文件信息
struct RankingFileInfo
{
    QString filename;               // 文件名称
    int pv;                         // 下载数
};
class Ranking : public QWidget
{
    Q_OBJECT

public:
    explicit Ranking(QWidget *parent = nullptr);
    ~Ranking();

    // 设置TableWidget表头和一些属性
    void initTableWidget();

    // ==========显示共享文件===========
    void refreshFiles();                                // 显示用户的文件列表
    void getUserFileList();                             // 获取新的文件列表信息  难
    void getFileJsonInfo(QByteArray data);              // 将获取到的数据解析处理
    void refreshList();                                 // 更新排行版列表
    void clearshareFileList();                          // 清空排行列表

    QByteArray setFileListJson(int start,int count);    // 生成获取共享文件json
    QString getCountStatus(QByteArray json);            // 获取服务器返回信息
signals:
    void loginAgainSignal();
private:
    Ui::Ranking *ui;
    Common m_common;
    QNetworkAccessManager* m_manager;
    LoginInfoInstance* m_login;

     long m_userFilesCount;       // 文件个数
     int m_start;                 // 请求获取开始个数
     int m_count;                 // 请求获取文件个数

     QList<RankingFileInfo *>m_list;        // 文件列表存储结点
};

#endif // RANKING_H
