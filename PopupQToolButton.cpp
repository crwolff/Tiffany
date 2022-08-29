#include "PopupQToolButton.h"
#include <QAction>

PopupQToolButton::PopupQToolButton(QWidget * parent) : QToolButton(parent)
{
    this->setPopupMode(QToolButton::MenuButtonPopup);
    this->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    QObject::connect(this, &QToolButton::triggered, this, &QToolButton::setDefaultAction);
}

PopupQToolButton::~PopupQToolButton()
{
}
