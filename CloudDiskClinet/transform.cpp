#include "transform.h"
#include "ui_transform.h"
#include <QFile>
#include "common/common.h"
#include "common/uploadlayout.h"
#include "common/logininfoinstance.h"

TransForm::TransForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransForm)
{
    ui->setupUi(this);

    // 设置上传布局实例
    UpLoadLayout *uploadLayout = UpLoadLayout::getInstance();
    uploadLayout->setUpLoadLayout(ui->upload_scroll);

    // 设置tab页
    connect(ui->tabWidget,&QTabWidget::currentChanged,[=](int index)
    {

    });

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
//            dispayDataRecord(); //显示数据传输记录
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
    ui->tabWidget->setCornerWidget(ui->download_list);
}
