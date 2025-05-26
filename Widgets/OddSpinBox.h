#ifndef ODDSPINBOX_H
#define ODDSPINBOX_H

#include <QSpinBox>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  class QRegularExpressionValidator;
#else
  class QRegExpValidator;
#endif

class OddSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    OddSpinBox(QWidget *parent = nullptr);

protected:
    QValidator::State validate(QString &text, int &pos) const;

private:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QRegularExpressionValidator *validator;
#else
    QRegExpValidator *validator;
#endif
};

#endif
