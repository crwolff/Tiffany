#include "DoubleSpinWidget.h"

DoubleSpinWidget::DoubleSpinWidget(float min, float max, float val, float step, QString text, QWidget * parent) : QWidget(parent)
{
    spinBox = new QDoubleSpinBox(this);
    spinBox->setMinimum(min);
    spinBox->setMaximum(max);
    spinBox->setValue(val);
    spinBox->setSingleStep(step);
    label = new QLabel(text, this);
    label->setAlignment(Qt::AlignCenter);
    layout = new QVBoxLayout(this);
    layout->addWidget(spinBox);
    layout->addWidget(label);
}

DoubleSpinWidget::~DoubleSpinWidget()
{
    delete layout;
    delete label;
    delete spinBox;
}
