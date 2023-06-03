#include "PopupQToolButton.h"
#include <QAction>

PopupQToolButton::PopupQToolButton(QWidget * parent) : QToolButton(parent)
{
    setPopupMode(QToolButton::MenuButtonPopup);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    QObject::connect(this, &QToolButton::triggered, this, &QToolButton::setDefaultAction);
}

PopupQToolButton::~PopupQToolButton()
{
}
