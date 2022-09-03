#ifndef TRANSFORM_H
#define TRANSFORM_H
#if _MSC_VER >=1600
#pragma execution_character_set("utf-8")
#endif
#include <QWidget>
#include "common/common.h"
namespace Ui {
class TransForm;
}

class TransForm : public QWidget
{
    Q_OBJECT

public:
    explicit TransForm(QWidget *parent = nullptr);
    ~TransForm();

    void dispayDataRecord(QString path = RECORDDIR);
    // 显示上传窗口
    void showUpload();
    // 显示下载窗口
    void showDownload();

signals:
    void currentTabSignal(QString); // 告诉主界面，当前是哪个tab ???

private:
    Ui::TransForm *ui;
};

#endif // TRANSFORM_H
