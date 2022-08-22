#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <QWidget>

namespace Ui {
class TransForm;
}

class TransForm : public QWidget
{
    Q_OBJECT

public:
    explicit TransForm(QWidget *parent = nullptr);
    ~TransForm();

private:
    Ui::TransForm *ui;
};

#endif // TRANSFORM_H
