#ifndef POPUPQTOOLBUTTON_H
#define POPUPQTOOLBUTTON_H

#include <QWidget>
#include <QToolButton>

class PopupQToolButton : public QToolButton
{
    Q_OBJECT

public:
    PopupQToolButton(QWidget * parent = NULL);
    ~PopupQToolButton();
};

#endif
