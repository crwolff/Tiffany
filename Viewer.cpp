#include "Viewer.h"
#include <QDebug>
#include <QPainter>
#include <QScrollBar>

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

//
// Replace paintEvent to get proper scaling of image
//
void Viewer::paintEvent(QPaintEvent *)
{
    if (currImage.isNull() == false)
    {
        QPainter p(this);
        p.setTransform(currTransform);
        p.drawImage(currImage.rect().topLeft(), currImage);
        p.end();
    }
}

// TODO
//
// Update viewer image from bookmarks
//
void Viewer::imageSelected(QListWidgetItem *curr, QListWidgetItem *)
{
    flushEdits();
    if (curr != NULL)
    {
        currListItem = curr;
        currImage = curr->data(Qt::UserRole).value<QImage>();
        fitToWindow();
    }
    else
    {
        currListItem = NULL;
        currImage = QImage();
    }
}

//
// Always honor aspect ratio when resizing
//
QSize Viewer::sizeHint() const
{
    if (currImage.isNull())
        return QWidget::sizeHint();
    return currImage.size() * scaleFactor * scaleBase;
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

//
// Pre-compute the scaling transformation matrix
//
void Viewer::setTransform()
{
    currTransform = QTransform();
    currTransform.scale(scaleFactor * scaleBase, scaleFactor * scaleBase);
    currInverse = currTransform.inverted();
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

//
// Helper function for window fit functions
//
bool Viewer::measureAll(int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH)
{
    // Get handle to parent's scrollbars
    if (scrollArea == NULL)
        scrollArea = (QScrollArea *)(parentWidget()->parentWidget());

    // Get thickness of scrollbars
    scrollBarSize = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    // Size of viewport not including scrollbars
    viewW = parentWidget()->width();
    if (scrollArea->verticalScrollBar()->isVisible())
        viewW += scrollBarSize;
    viewH = parentWidget()->height();
    if (scrollArea->horizontalScrollBar()->isVisible())
        viewH += scrollBarSize;

    // Size of image
    imageW = currImage.size().width();
    imageH = currImage.size().height();

    // Scale to horizontal if it is larger
    if (viewW * imageH > viewH * imageW)
        return true;
    return false;
}

//
// Fit image to window without scrollbars
//
void Viewer::fitToWindow()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    scaleBase = 1.0;
    scaleFactor = 1.0;
    if (currImage.isNull())
        return;

    // Scale to larger dimension
    if (measureAll(scrollBarSize, viewW, viewH, imageW, imageH))
        scaleBase = (float)viewH / imageH;
    else
        scaleBase = (float)viewW / imageW;
    setTransform();

    // Update scrollarea
    updateGeometry();
    emit zoomSig();
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
