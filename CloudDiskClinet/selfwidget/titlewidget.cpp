#include "titlewidget.h"
#include "ui_titlewidget.h"
#include <QMouseEvent>
#include <QWidget>
#pragma execution_character_set("utf-8")

TitleWidget::TitleWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::titlewidget)
{
    ui->setupUi(this);

    // 加载logo
    ui->logo->setPixmap(QPixmap(":/image/logo.png").scaled(20,20));         // 设置logo

    m_parent = parent;                                                      // 直接从父进程获取对象 只限启动窗口的子进程

    // 按钮功能实现
    connect(ui->set,&QToolButton::clicked,[=]()
    {
        // 发送自定义信号
        emit showSetWg();
    });

    // 实现关闭
    connect(ui->close,&QToolButton::clicked,[=]()
    {
        emit closeWindow();

    });

    // 实现最小化按钮
    connect(ui->min,&QToolButton::clicked,[=]()     // 通过父框对象进行关闭
    {
        m_parent->showMinimized();
    });
}

TitleWidget::~TitleWidget()
{
    delete ui;
}

// 鼠标处理 如果在login上进行编写 那鼠标在任意位置都可以移动
void TitleWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 只允许左键拖动
    if(event->buttons() & Qt::LeftButton)        // button处理单机 button处理长时间操作
    {
        // 窗口跟随鼠标移动
        // 窗口左上角点 = 鼠标当前位置 - 差值
        m_parent->move(event->globalPos()-m_pt);
    }
}

void TitleWidget::mousePressEvent(QMouseEvent *event)
{
    // 如果鼠标左键按下
    if(event->button() == Qt::LeftButton)
    {
        // 求差值 : 鼠标当前位置-窗口左上角
        m_pt = event->globalPos() - m_parent->geometry().topLeft();
    }
}
