#ifndef BUTTONGROUP_H
#define BUTTONGROUP_H

#include <QWidget>
#include <QMap>
#include <QSignalMapper>
#include <QToolButton>
namespace Ui {
class ButtonGroup;
}


enum class Page{MYFILE, SHARE, TRANKING, TRANSFER, SWITCHUSR};

class ButtonGroup : public QWidget
{
    Q_OBJECT

public:
    explicit ButtonGroup(QWidget *parent = nullptr);
    ~ButtonGroup();

    void defaulfPage();
signals:
    void sigMyFile();
    void sigShareFile();
    void sigDownload();
    void sigTransform();
    void sigSwitchUser();
    void closeWindow();
    void minWindow();
    void maxWindow();

public slots:
    void setParent(QWidget *parent);                    // 设置记录父框对象
    void slotButtonClick(Page cur);
    void slotButtonClick(QString text);

protected:
    void mouseMoveEvent(QMouseEvent *event);            // 设置鼠标移动
    void mousePressEvent(QMouseEvent *event);           // 获取鼠标差值
    void paintEvent(QPaintEvent *event);                // 设置标题框图片

private:
    Ui::ButtonGroup *ui;

    QPoint m_pt;                                        // 鼠标计量差值
    QWidget* m_parent;                                  // 记录父框对象

    QToolButton* m_curBtn;                              //
    QMap<QString,QToolButton*> m_btns;                  // 记录按钮对应的page
    QMap<Page,QString> m_pages;                         // 记录page对应的名称
    QSignalMapper *m_mapper;                            // 信号列表
};

#endif // BUTTONGROUP_H
