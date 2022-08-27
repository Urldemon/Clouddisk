#include "uploadlayout.h"
#include <QDebug>
#include "common.h"

UpLoadLayout* UpLoadLayout::instance = new UpLoadLayout;

UpLoadLayout::Garbo UpLoadLayout::garbo;

UpLoadLayout::UpLoadLayout()
{
}

UpLoadLayout *UpLoadLayout::getInstance()
{
    return instance;
}

void UpLoadLayout::destroy()
{
    if(UpLoadLayout::instance != nullptr)
    {
        delete instance;
        instance = nullptr;
        cout << "instance is delete";
    }
}

void UpLoadLayout::setUpLoadLayout(QWidget *p)
{
    m_wg = new QWidget(p);
    // 创建
    QLayout* layout = p->layout();
    layout->addWidget(m_wg);        // 添加对象
    layout->setContentsMargins(0,0,0,0);

    // 创建布局
    QVBoxLayout* vlayout = new QVBoxLayout;
    // 布局设置给窗口
    m_wg->setLayout(vlayout);
    // 边界间隔
    vlayout->setContentsMargins(0,0,0,0);
    m_layout = vlayout;

    // 添加弹簧
    vlayout->addStretch();
}

// 获取布局
QLayout *UpLoadLayout::getUpLoadLayout()
{
    return m_layout;
}

