#include "SpinWidget.h"

SpinWidget::SpinWidget(int min, int max, int val, QString text, QWidget * parent) : QWidget(parent)
{
    spinBox = new QSpinBox(this);
    spinBox->setMinimum(min);
    spinBox->setMaximum(max);
    spinBox->setValue(val);
    label = new QLabel(text, this);
    label->setAlignment(Qt::AlignCenter);
    layout = new QVBoxLayout(this);
    layout->addWidget(spinBox);
    layout->addWidget(label);
}

SpinWidget::~SpinWidget()
{
    delete layout;
    delete label;
    delete spinBox;
}
