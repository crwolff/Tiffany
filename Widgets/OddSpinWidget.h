#ifndef ODDSPINWIDGET_H
#define ODDSPINWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

class OddSpinWidget : public QWidget
{
    Q_OBJECT

public:
    OddSpinWidget(int min, int max, int val, int step, QString text, QWidget * parent=nullptr);
    ~OddSpinWidget();

    QSpinBox *spinBox;
    QLabel *label;
    QVBoxLayout *layout;
};

#endif // ODDSPINWIDGET_H
