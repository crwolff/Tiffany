#include "Viewer.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

Viewer::Viewer(QWidget * parent) : QWidget(parent)
{
}

Viewer::~Viewer()
{
}

//
// Handle mouse button down
//
void Viewer::mousePressEvent(QMouseEvent *event)
{
    bool flag = false;

    // Create rubberBand
    if (rubberBand == NULL)
        rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

    // If left mouse button is pressed
    if (event->button() == Qt::LeftButton)
    {
        origin = event->pos();
        if ((leftMode == "Draw") || (leftMode == "Erase"))
        {
            pushImage();
            drawing = true;
            setCursor(Qt::CrossCursor);
        }
        else // Pointer or Fill
        {
            rubberBand->setGeometry(QRect(origin, QSize()));
            rubberBand->show();
        }
        flag = true;
    }

    // If right mouse button pressed
    if (event->button() == Qt::RightButton)
    {
        origin = event->pos();
        rubberBand->setGeometry(QRect(origin, QSize()));
        rubberBand->show();
        flag = true;
    }

    // Event was handled
    if (flag)
        event->accept();
    else
        QWidget::mousePressEvent(event);
}

//
// Handle mouse motion
//
void Viewer::mouseMoveEvent(QMouseEvent *event)
{
    bool flag = false;

    // If left mouse button is pressed
    if (event->buttons() & Qt::LeftButton)
    {
        if ((leftMode == "Pointer") || (leftMode == "Fill"))
        {
            rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
            flag = true;
        }
        else if (drawing)
        {
            QColor color = (leftMode == "Draw") ? foregroundColor : backgroundColor;
            drawLine(origin, event->pos(), color);
            origin = event->pos();
            flag = true;
        }
    }

    // If right mouse button pressed
    if (event->buttons() & Qt::RightButton)
    {
        rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
        flag = true;
    }

    // Event was handled
    if (flag)
        event->accept();
    else
        QWidget::mouseMoveEvent(event);
}

//
// Cleanup after mouse action
//
void Viewer::mouseReleaseEvent(QMouseEvent *event)
{
    bool flag = false;

    // If left mouse button was released
    if (event->button() == Qt::LeftButton)
    {
        if (leftMode == "Fill")
        {
            rubberBand->hide();
            fillArea(rubberBand->geometry());
            currListItem->setData(Qt::UserRole, currImage);
            currListItem->setData(Qt::UserRole+2, currListItem->data(Qt::UserRole+2).value<int>() + 1);
            emit imageChangedSig();
        }
        else if (drawing)
        {
            drawing = false;
            setCursor(Qt::ArrowCursor);
            currListItem->setData(Qt::UserRole, currImage);
            currListItem->setData(Qt::UserRole+2, currListItem->data(Qt::UserRole+2).value<int>() + 1);
            emit imageChangedSig();
        }
        flag = true;
    }

    // If right mouse button was released
    if (event->button() == Qt::RightButton)
    {
        rubberBand->hide();
        zoomArea(rubberBand->geometry());
        flag = true;
    }

    // Event was handled
    if (flag)
        event->accept();
    else
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
    update();
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

//
// Select pointer tool
//
void Viewer::pointerMode()
{
    leftMode = "Pointer";
}

//
// Select draw line tool
//
void Viewer::pencilMode()
{
    leftMode = "Draw";
}

//
// Select erase line tool
//
void Viewer::eraserMode()
{
    leftMode = "Erase";
}

//
// Select erase/fill area tool
//
void Viewer::areaFillMode()
{
    leftMode = "Fill";
}

//
// Select narrow brush
//
void Viewer::setBrush_1()
{
    brushSize = 1;
}

//
// Select medium brush
//
void Viewer::setBrush_4()
{
    brushSize = 4;
}

//
// Select wide brush
//
void Viewer::setBrush_8()
{
    brushSize = 8;
}

//
// Select extra-wide brush
//
void Viewer::setBrush_12()
{
    brushSize = 12;
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

//
// Draw/Erase a line in the foreground/background color
//
void Viewer::drawLine(QPoint start, QPoint finish, QColor color)
{
    if (currImage.isNull())
        return;
    QPainter p(&currImage);
    p.setTransform(currInverse);
    qreal brush = int(brushSize * scaleFactor * scaleBase);
    p.setPen(QPen(color, brush, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(start, finish);
    p.end();
    update();
}

//
// Fill area with background color
//
void Viewer::fillArea(QRect rect)
{
    if (currImage.isNull())
        return;
    QPainter p(&currImage);
    p.setTransform(currInverse);
    p.fillRect(rect, backgroundColor);
    p.end();
    update();
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

//
// Force QScrollArea to recalculate scrollbars
//
void Viewer::updateScrollBars()
{
    scrollArea->setWidgetResizable(true);
}

//
// Recenter view after scaling change
//
void Viewer::adjustScrollBars(float factor)
{
    scrollArea->horizontalScrollBar()->setValue(int(
                factor * scrollArea->horizontalScrollBar()->value() +
                ((factor - 1) * scrollArea->horizontalScrollBar()->pageStep() / 2)));
    scrollArea->verticalScrollBar()->setValue(int(
                factor * scrollArea->verticalScrollBar()->value() +
                ((factor - 1) * scrollArea->verticalScrollBar()->pageStep() / 2)));
}

//
// Draw image 25% larger
//
void Viewer::zoomIn()
{
    if (currImage.isNull())
        return;
    scaleFactor = scaleFactor * 1.25;
    setTransform();
    updateGeometry();
    updateScrollBars();
    adjustScrollBars(1.25);
    emit zoomSig();
}

//
// Draw image 20% smaller
void Viewer::zoomOut()
{
    if (currImage.isNull())
        return;
    scaleFactor = scaleFactor * 0.8;
    setTransform();
    updateGeometry();
    updateScrollBars();
    adjustScrollBars(0.8);
    emit zoomSig();
}

//
// Zoom to rubberband rectangle
//
void Viewer::zoomArea(QRect rect)
{
    if (currImage.isNull())
        return;

    // Get center of zoom rectangle
    float centerX = (float)rect.center().x() / geometry().width();
    float centerY = (float)rect.center().y() / geometry().height();

    // Size of viewport with scrollbars on
    int scrollBarSize = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int viewW = parentWidget()->width();
    if (!scrollArea->verticalScrollBar()->isVisible())
        viewW -= scrollBarSize;
    int viewH = parentWidget()->height();
    if (!scrollArea->horizontalScrollBar()->isVisible())
        viewH -= scrollBarSize;

    // Scale to larger dimension
    int rectW = rect.width();
    int rectH = rect.height();
    if ((rectW * viewH) > (rectH * viewW))
        scaleFactor *= viewW / rectW;
    else
        scaleFactor *= viewH / rectH;
    setTransform();
    updateGeometry();
    updateScrollBars();

    // Adjust scrollbars so center of rect is center of viewport
    scrollArea->horizontalScrollBar()->setValue(int(
                centerX * scrollArea->horizontalScrollBar()->maximum() +
                ((centerX - 0.5) * scrollArea->horizontalScrollBar()->pageStep())));
    scrollArea->verticalScrollBar()->setValue(int(
                centerY * scrollArea->verticalScrollBar()->maximum() +
                ((centerY - 0.5) * scrollArea->verticalScrollBar()->pageStep())));
    emit zoomSig();
}

//
// Helper function for window fit functions
//     returns true if horizontal dimension is larger
//
bool Viewer::measureAll(int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH)
{
    // Get handle to parent's scrollbars
    if (scrollArea == NULL)
        scrollArea = (QScrollArea *)(parentWidget()->parentWidget());

    // Get thickness of scrollbars
    scrollBarSize = style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    // Size of viewport with scrollbars off
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

//
// Fit image to window with only a vertical scrollbar
//
void Viewer::fitWidth()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    scaleBase = 1.0;
    scaleFactor = 1.0;
    if (currImage.isNull())
        return;

    // If height is larger dimension, leave space for vertical scroll bar
    if (measureAll(scrollBarSize, viewW, viewH, imageW, imageH))
        scaleBase = (float)(viewW - scrollBarSize) / imageW;
    else
        scaleBase = (float)viewW / imageW;
    setTransform();

    // Update scrollarea
    updateGeometry();
    emit zoomSig();
}

//
// Fit image to window with only a horizontal scrollbar
//
void Viewer::fitHeight()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    scaleBase = 1.0;
    scaleFactor = 1.0;
    if (currImage.isNull())
        return;

    // If width is larger dimension, leave space for horizontal scroll bar
    if (measureAll(scrollBarSize, viewW, viewH, imageW, imageH))
        scaleBase = (float)(viewH - scrollBarSize) / imageH;
    else
        scaleBase = (float)viewH / imageH;
    setTransform();

    // Update scrollarea
    updateGeometry();
    emit zoomSig();
}

//
// Fit image to window with a maximum of one scrollbar
//
void Viewer::fillWindow()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    scaleBase = 1.0;
    scaleFactor = 1.0;
    if (currImage.isNull())
        return;

    // Scale to smaller dimension
    if (measureAll(scrollBarSize, viewW, viewH, imageW, imageH))
        fitWidth();
    else
        fitHeight();
}
