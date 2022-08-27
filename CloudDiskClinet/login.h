#ifndef LOGIN_H
#define LOGIN_H
#pragma execution_character_set("utf-8")

#include <QDialog>
#include <QNetworkAccessManager>
#include <mainwindow.h>
#include "common/common.h"
#include "common/aeskeyinstance.h"

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

    QByteArray SetSgininJson(QString user,QString pwd);                                                     // 将登录信息打包
    QByteArray SetResginJson(QString user, QString nickname, QString pwd, QString phone, QString email);    //
    QStringList getSgininStatus(QByteArray json);

private slots:
    void on_sginin_btn_clicked();

    void on_resgin_confirm_btn_clicked();

    void on_ser_ok_btn_clicked();


private:
    void readCfg();                             // 获取本地存储信息

private:
    Ui::Login *ui;

    // mainwindows对象
    MainWindow* m_mainwin;
    // 处理请求的管理员
    QNetworkAccessManager* m_manager;
    // 管理对象
    Common m_common;
    // 加密对象
    AesKeyInstance* m_aeskey;
protected:
    // 绘图函数
    void paintEvent(QPaintEvent *event);    // 操作系统的事
};

#endif // LOGIN_H
