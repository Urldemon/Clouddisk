#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "transform.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 去除边框
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    // 给btn_group窗口设置父窗口类对象
    ui->btn_group->setParent(this);

    // 处理信号
    managerSignals();

    // 默认显示我的文件窗口
    ui->stackedWidget->setCurrentWidget(ui->myfiles_page);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showMainWindos()
{
    // 输出界面
    this->show();
    // 将显示窗口挪到桌面中央
    m_common.moveToCenter(this);

    // 设置为默认页面
    ui->btn_group->defaulfPage();

    // 切换到我的文件页面
    ui->stackedWidget->setCurrentWidget(ui->myfiles_page);
}

void MainWindow::managerSignals()
{
    // 获取到关闭信号
    connect(ui->btn_group,&ButtonGroup::closeWindow,this,&MainWindow::close);
    // 获取到缩小信号
    connect(ui->btn_group,&ButtonGroup::minWindow,this,&MainWindow::showMinimized);
    // 获取到放大信号
    connect(ui->btn_group,&ButtonGroup::maxWindow,[=]()
    {
        static bool flag = false;
        if(flag == true)
        {
            this->showNormal();
            flag = false;
        }
        else
        {
            this->showMaximized();
            flag = true;
        }
    });

    // 用户信息
    connect(ui->btn_group,&ButtonGroup::sigUserDate,[=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->userdate_page);
    });

    // 我的文件
    connect(ui->btn_group, &ButtonGroup::sigMyFile, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->myfiles_page);

    });
    // 分享列表
    connect(ui->btn_group, &ButtonGroup::sigShareFile, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->sharefile_page);
        // 刷新分享列表
    });
    // 下载榜
    connect(ui->btn_group, &ButtonGroup::sigRanking, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->ranking_page);
        // 刷新下载榜列表
        ui->ranking_page->refreshFiles();

    });
    // 传输列表
    connect(ui->btn_group, &ButtonGroup::sigTransForm, [=]()
    {
        ui->stackedWidget->setCurrentWidget(ui->transfer_page);
    });
    // 切换用户
    connect(ui->btn_group, &ButtonGroup::sigSwitchUser, [=]()
    {
        qDebug() << "bye bye...";
        showLogin();
    });

    // 登录出错重新登录
    connect(ui->myfiles_page,&MyFile::loginAgainSignal,[=]()
    {
        qDebug() << "bye bye...";
        showLogin();
    });

    // stack窗口切换
    // 下载操作
    connect(ui->myfiles_page,&MyFile::gotoTransfer,[=](TransferStatus status)
    {
        ui->btn_group->slotButtonClick(Page::TRANSFER);

//        ui->stackedWidget->setCurrentWidget(ui->transfer_page);   // 按钮没有发生变化
        if(status == TransferStatus::Uplaod)
        {
             ui->transfer_page->showUpload();
        }
        else if(status == TransferStatus::Download)
        {
            ui->transfer_page->showDownload();
        }
    });

}

void MainWindow::showLogin()
{
    // 发送信号
    emit changeUser();


}
