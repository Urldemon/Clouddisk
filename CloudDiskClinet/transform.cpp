#include "transform.h"
#include "ui_transform.h"

TransForm::TransForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TransForm)
{
    ui->setupUi(this);
}

TransForm::~TransForm()
{
    delete ui;
}
