#ifndef UPLOADLAYOUT_H
#define UPLOADLAYOUT_H

#include <QWidget>
#include <QVBoxLayout>

// 上传进度条布局，单例模式 没有继承
class UpLoadLayout
{
public:

    static UpLoadLayout *getInstance();
    static void destroy();

    void setUpLoadLayout(QWidget *p);
    QLayout* getUpLoadLayout();
private:
    UpLoadLayout();

    static UpLoadLayout* instance;
    class Garbo
    {
    public:
        ~Garbo()
        {
            UpLoadLayout::destroy;
        }
    };
    static Garbo garbo;

private:
       QWidget *m_wg;
       QLayout *m_layout;
};

#endif // UpLoadLayout_H
