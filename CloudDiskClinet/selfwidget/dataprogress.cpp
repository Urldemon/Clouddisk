#include "dataprogress.h"
#include "ui_dataprogress.h"

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
    ui->label->setText(name + ":");
    ui->progressBar->setMinimum(0);         // 最小值
    ui->progressBar->setValue(0);           // 将值置为0
}

// 设置进度条当前值和最大值
void DataProgress::setProgress(int value, int max)
{
    ui->progressBar->setValue(value);
    ui->progressBar->setMaximum(max);
}

