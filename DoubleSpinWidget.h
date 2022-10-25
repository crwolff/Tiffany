#ifndef DOUBLESPINWIDGET_H
#define DOUBLESPINWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QVBoxLayout>

class DoubleSpinWidget : public QWidget
{
    Q_OBJECT

public:
    DoubleSpinWidget(float min, float max, float val, float step, QString text, QWidget * parent=nullptr);
    ~DoubleSpinWidget();

    QDoubleSpinBox *spinBox;
    QLabel *label;
    QVBoxLayout *layout;
};

#endif // DOUBLESPINWIDGET_H
