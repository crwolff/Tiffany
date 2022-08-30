#include "Viewer.h"

Viewer::Viewer(QWidget * parent) : QWidget(parent)
{
}

Viewer::~Viewer()
{
}

// TODO
void Viewer::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
}

// TODO
void Viewer::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

// TODO
void Viewer::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
}

// TODO
void Viewer::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

// TODO
void Viewer::imageSelected(QListWidgetItem *curr, QListWidgetItem *)
{
}

// TODO
QSize Viewer::sizeHint() const
{
    return QWidget::sizeHint();
}

// TODO
void Viewer::pointerMode()
{
}

// TODO
void Viewer::pencilMode()
{
}

// TODO
void Viewer::eraserMode()
{
}

// TODO
void Viewer::areaFillMode()
{
}

// TODO
void Viewer::setBrush()
{
}

// TODO
void Viewer::setTransform()
{
}

// TODO
void Viewer::drawLine(QPoint *start, QPoint *finish, QColor color)
{
}

// TODO
void Viewer::fillArea(QRect *rect)
{
}

// TODO
void Viewer::flushEdits()
{
}

// TODO
void Viewer::pushImage()
{
}

// TODO
void Viewer::undoEdit()
{
}

// TODO
void Viewer::redoEdit()
{
}

// TODO
void Viewer::updateScrollBars()
{
}

// TODO
void Viewer::adjustScrollBars(float factor)
{
}

// TODO
void Viewer::zoomIn()
{
}

// TODO
void Viewer::zoomOut()
{
}

// TODO
void Viewer::zoomArea()
{
}

// TODO
bool Viewer::measureAll()
{
    return false;
}

// TODO
void Viewer::fitToWindow()
{
}

// TODO
void Viewer::fitWidth()
{
}

// TODO
void Viewer::fitHeight()
{
}

// TODO
void Viewer::fillWindow()
{
}
