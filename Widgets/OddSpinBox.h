#ifndef ODDSPINBOX_H
#define ODDSPINBOX_H

#include <QSpinBox>

class QRegExpValidator;

class OddSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    OddSpinBox(QWidget *parent = nullptr);

protected:
    QValidator::State validate(QString &text, int &pos) const;

private:
    QRegExpValidator *validator;
};

#endif
