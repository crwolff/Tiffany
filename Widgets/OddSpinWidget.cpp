#include "OddSpinBox.h"
#include "OddSpinWidget.h"

OddSpinWidget::OddSpinWidget(int min, int max, int val, int step, QString text, QWidget * parent) : QWidget(parent)
{
    spinBox = new OddSpinBox(this);
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

OddSpinWidget::~OddSpinWidget()
{
    delete layout;
    delete label;
    delete spinBox;
}
