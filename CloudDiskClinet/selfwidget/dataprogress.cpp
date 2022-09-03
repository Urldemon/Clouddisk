#include "dataprogress.h"
#include "ui_dataprogress.h"
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QPainter>

DataProgress::DataProgress(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataProgress)
{
    ui->setupUi(this);
}

DataProgress::~DataProgress()
{
    delete ui;
}


void DataProgress::setFileName(QString name)
{
    ui->progressBar->setMinimum(0);         // 最小值
    ui->progressBar->setValue(0);           // 将值置为0

    // 设置文本大小
    QFontMetrics fontmetrics(ui->label->font());
    QString str = fontmetrics.elidedText(name,Qt::ElideRight,ui->label->width());   // 设置文字大小不超过label框大小
    ui->label->setText(str + " : ");
    ui->label->setToolTip(name);                                                    // 鼠标指向显示全名
}

// 设置进度条当前值和最大值
void DataProgress::setProgress(int value, int max)
{
    ui->progressBar->setValue(value);
    ui->progressBar->setMaximum(max);
}


