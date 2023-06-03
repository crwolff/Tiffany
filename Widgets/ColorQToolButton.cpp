#include "ColorQToolButton.h"
#include <QPixmap>

ColorQToolButton::ColorQToolButton(QWidget * parent) : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setMouseTracking(true);
    mode = "Foreground";
}

ColorQToolButton::~ColorQToolButton()
{
}

void ColorQToolButton::setIcon(QColor foregroundColor, QColor backgroundColor)
{
    QPixmap qpix = QPixmap(":/images/assets/fg_color.ico");
    QPainter painter(&qpix);
    painter.setBrush(Qt::SolidPattern);
    painter.fillRect(1,1,34,34,backgroundColor);
    painter.fillRect(29,29,62,62,foregroundColor);
    painter.end();
    QToolButton::setIcon(QIcon(qpix));
}

void ColorQToolButton::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->pos().x() <= 25) && (event->pos().y() <= 15))
    {
        setToolTip("Set background color");
        mode = "Background";
    }
    else if ((event->pos().x() > 25) && (event->pos().y() <= 15))
    {
        setToolTip("Swap foreground/background colors");
        mode = "Swap";
    }
    else if (event->pos().x() <= 25)
    {
        setToolTip("Reset colors to default");
        mode = "Reset";
    }
    else if (event->pos().x() > 25)
    {
        setToolTip("Set foreground color");
        mode = "Foreground";
    }
    QToolButton::mouseMoveEvent(event);
}
