#include "transform.h"
#include "ui_transform.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QFile>
#include "common/common.h"
#include "common/uploadlayout.h"
#include "common/logininfoinstance.h"
#include "common/downloadlayout.h"

TransForm::TransForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransForm)
{
    ui->setupUi(this);

    // 设置上传布局实例
    UpLoadLayout *uploadLayout = UpLoadLayout::getInstance();
    uploadLayout->setUpLoadLayout(ui->upload_scroll);

    // 设置下载布局实例
    DownloadLayout *downloadLayout = DownloadLayout::getInstance();
    downloadLayout->setDownloadLayout(ui->download_scroll);

    ui->tabWidget->setCurrentIndex(0);

    // 切换tab页
    connect(ui->tabWidget, &QTabWidget::currentChanged, [=](int index)
    {
        if(index == 0) //上传
        {
            emit currentTabSignal("正在上传");
        }
        else if(index == 1)//下载
        {
             emit currentTabSignal("正在下载");
        }
        else //传输记录
        {
            emit currentTabSignal("传输记录");
            dispayDataRecord(); //显示数据传输记录
        }
    });


    // 设置清空记录按钮
    connect(ui->clearBtn,&QToolButton::clicked,[=]()
    {
        // 获取登录信息
        LoginInfoInstance *login = LoginInfoInstance::getInstance();

        // 本地信息存储位置
        QString filename = QString(RECORDDIR + login->getUser());

        if(QFile::exists(filename) == true)     // 查看是否有文件
        {
            QFile::remove(filename);        // 删除文件
            ui->record_msg->clear();
            cout << "清除成功";
        }
        else cout << "没有文件";
    });
}

void TransForm::dispayDataRecord(QString path)
{
    //获取登陆信息实例
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); //获取单例

    //文件名字，登陆用户名则为文件名
    QString fileName = path + login->getUser();
    QFile file(fileName);

    if( false == file.open(QIODevice::ReadOnly) ) //只读方式打开
    {
        cout << "file.open(QIODevice::ReadOnly) err";
        return;
    }

    QByteArray array = file.readAll();

    #ifdef _WIN32 //如果是windows平台
        //fromLocal8Bit(), 本地字符集转换为utf-8
        ui->record_msg->setText( QString::fromLocal8Bit(array) );
    #else //其它平台
        ui->record_msg->setText( array );
    #endif

        file.close();
}

TransForm::~TransForm()
{
    delete ui;
}

void TransForm::showUpload()
{
    ui->tabWidget->setCurrentWidget(ui->upload_list);
}

void TransForm::showDownload()
{

    ui->tabWidget->setCurrentWidget(ui->download_list);
}
