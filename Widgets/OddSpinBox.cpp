#include <QtGui>

#include "OddSpinBox.h"

OddSpinBox::OddSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    validator = new QRegularExpressionValidator(QRegularExpression("[0-9]*[13579]"), this);
#else
    validator = new QRegExpValidator(QRegExp("[0-9]*[13579]"), this);
#endif
}

QValidator::State OddSpinBox::validate(QString &text, int &pos) const
{
    return validator->validate(text, pos);
}
