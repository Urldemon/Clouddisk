#include "buttongroup.h"
#include "ui_buttongroup.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif


#include <QMouseEvent>
#include <QDebug>
#include <QPainter>

ButtonGroup::ButtonGroup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ButtonGroup)
{
    ui->setupUi(this);

    // 初始化
    m_mapper = new QSignalMapper(this);
    m_curBtn = ui->myfile;
    m_curBtn->setStyleSheet("color:blue");

    // 按钮提示内容：按钮指针
    m_btns.insert(ui->loginuser->text(),ui->loginuser);
    m_btns.insert(ui->myfile->text(),ui->myfile);
    m_btns.insert(ui->sharefile->text(),ui->sharefile);
    m_btns.insert(ui->ranking->text(),ui->ranking);
    m_btns.insert(ui->transform->text(),ui->transform);
    m_btns.insert(ui->switchuser->text(),ui->switchuser);

    m_pages.insert(Page::USERDATE,ui->loginuser->text());
    m_pages.insert(Page::MYFILE, ui->myfile->text());
    m_pages.insert(Page::SHARE, ui->sharefile->text());
    m_pages.insert(Page::TRANKING , ui->ranking->text());
    m_pages.insert(Page::TRANSFER, ui->transform->text());
    m_pages.insert(Page::SWITCHUSR, ui->switchuser->text());

//    QMap<QString, QToolButton*>::iterator it = m_btns.begin();
//    for(; it != m_btns.end(); ++it)
//    {
//        m_mapper->setMapping(it.value(), it.value()->text());
//        connect(it.value(), SIGNAL(clicked(bool)), m_mapper, SLOT(map()));
//    }
    for(auto it:m_btns)
    {
        m_mapper->setMapping(it,it->text());
        connect(it,SIGNAL(clicked(bool)),m_mapper,SLOT(map()));
    }
    connect(m_mapper,SIGNAL(mapped(QString)),this,SLOT(slotButtonClick(QString)));

    // 关闭窗口
    connect(ui->close,&QToolButton::clicked,[=]()
    {
        emit closeWindow();
    });
    // 最小化
    connect(ui->min,&QToolButton::clicked,[=]()
    {
        emit minWindow();
    });
    // 最大化和还原
    connect(ui->max,&QToolButton::clicked,[=]()
    {
        static bool fl = false;
        if(fl == true)
            ui->max->setIcon(QIcon(":/image/title_max.png"));
        else
            ui->max->setIcon(QIcon(":/image/title_normal.png"));

        fl = !fl;
        emit maxWindow();
    });


}

ButtonGroup::~ButtonGroup()
{
    delete m_mapper;
    delete ui;
}

void ButtonGroup::defaulfPage()
{
    m_curBtn->setStyleSheet("color:black");
    m_curBtn = ui->myfile;
    m_curBtn->setStyleSheet("color:blue");
}

void ButtonGroup::setParent(QWidget *parent)
{
    m_parent = parent;
}

void ButtonGroup::slotButtonClick(Page cur)
{
    QString text = m_pages[cur];
    slotButtonClick(text);
}

void ButtonGroup::slotButtonClick(QString text)
{
//    qDebug() << "============" << text ;
    // 获取按钮对应page
    QToolButton* btn = m_btns[text];

    if(btn == m_curBtn && btn != ui->switchuser)return;
    // 将对应的按钮显示选择对象
    m_curBtn->setStyleSheet("color:black");
    btn->setStyleSheet("color:blue");
    m_curBtn = btn;
    //
    if(text == ui->loginuser->text())
        emit sigUserDate();
    else if(text == ui->myfile->text())
        emit sigMyFile();
    else if(text == ui->sharefile->text())
        emit sigShareFile();
    else if(text == ui->ranking->text())
        emit sigRanking();
    else if(text == ui->transform->text())
        emit sigTransForm();
    else if(text == ui->switchuser->text())
        emit sigSwitchUser();
}

void ButtonGroup::mouseMoveEvent(QMouseEvent *event)
{
    // 判断是否是鼠标左键长时间按状态
    if(event->buttons() & Qt::LeftButton)
    {
        // 移动窗口位置 当前鼠标位置 - 差值
        m_parent->move(event->globalPos() - m_pt);
    }
}

void ButtonGroup::mousePressEvent(QMouseEvent *event)
{
    // 当是鼠标左键按下时状态
    if(event->button() == Qt::LeftButton)
    {
        // 差值 = 当前鼠标位置 - 当前窗口最左上角位置
        m_pt = event->globalPos() - m_parent->geometry().topLeft();
    }
}

void ButtonGroup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0,0,width(),height(),QPixmap(":/image/title_bk.jpg"));

}


