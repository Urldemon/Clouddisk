#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QMainWindow>
#include "common/common.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void showMainWindos();

    void managerSignals();

    void showLogin();

signals:
    // 切换用户
    void changeUser();


private:
    Ui::MainWindow *ui;

    Common m_common;

protected:
    void  painEvent(QPaintEvent *event);
};
#endif // MAINWINDOW_H
