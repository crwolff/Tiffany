#include <QtGui>

#include "OddSpinBox.h"

OddSpinBox::OddSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    validator = new QRegExpValidator(QRegExp("[0-9]*[13579]"), this);
}

QValidator::State OddSpinBox::validate(QString &text, int &pos) const
{
    return validator->validate(text, pos);
}
