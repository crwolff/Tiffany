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
//                self.currListItem.setData(Defines.roleImage, self.currImage)
//                self.currListItem.setData(Defines.roleChanges, self.currListItem.data(Defines.roleChanges) + 1)
//                self.imageChangedSig.emit()
        }
        else if (drawing)
        {
            drawing = false;
            setCursor(Qt::ArrowCursor);
//                self.currListItem.setData(Defines.roleImage, self.currImage)
//                self.currListItem.setData(Defines.roleChanges, self.currListItem.data(Defines.roleChanges) + 1)
//                self.imageChangedSig.emit()
        }
        flag = true;
    }

    // If right mouse button was released
    if (event->button() == Qt::RightButton)
    {
        zoomArea();
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

// TODO
void Viewer::drawLine(QPoint start, QPoint finish, QColor color)
{
    qInfo() << "drawLine" << start << finish << color;
}

// TODO
void Viewer::fillArea(QRect rect)
{
    qInfo() << "fillArea" << rect;
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
    adjustScrollBars(0.8);
    emit zoomSig();
}

// TODO
void Viewer::zoomArea()
{
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
