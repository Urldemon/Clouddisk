#ifndef TITLEWIDGET_H
#define TITLEWIDGET_H

#include <QWidget>

namespace Ui {
class titlewidget;
}

class TitleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleWidget(QWidget *parent = nullptr);
    ~TitleWidget();

protected:
    void mouseMoveEvent(QMouseEvent *event);            // 设置鼠标移动
    void mousePressEvent(QMouseEvent *event);           // 获取鼠标差值

signals:
    void showSetWg();                                   // 设置信号
    void closeWindow();                                 // 关闭信号

private:
    Ui::titlewidget *ui;

    // 存储鼠标差值
    QPoint m_pt;

    QWidget* m_parent;

};

#endif // TITLEWIDGET_H
