#ifndef DATAPROGRESS_H
#define DATAPROGRESS_H
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QWidget>

namespace Ui {
class DataProgress;
}

class DataProgress : public QWidget
{
    Q_OBJECT

public:
    explicit DataProgress(QWidget *parent = nullptr);
    ~DataProgress();

    void setFileName(QString name);
    void setProgress(int value,int max);
private:
    Ui::DataProgress *ui;
};

#endif // DATAPROGRESS_H
