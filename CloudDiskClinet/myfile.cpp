#include "myfile.h"
#include "ui_myfile.h"

MyFile::MyFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyFile)
{
    ui->setupUi(this);
}

MyFile::~MyFile()
{
    delete ui;
}
