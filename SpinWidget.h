#ifndef SPINWIDGET_H
#define SPINWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

class SpinWidget : public QWidget
{
    Q_OBJECT

public:
    SpinWidget(int min, int max, int val, QString text, QWidget * parent=nullptr);
    ~SpinWidget();

    QSpinBox *spinBox;
    QLabel *label;
    QVBoxLayout *layout;
};

#endif // SPINWIDGET_H
