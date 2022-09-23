#include "Viewer.h"
#include "UndoBuffer.h"
#include <QDebug>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

Viewer::Viewer(QWidget * parent) : QWidget(parent)
{
    setFocusPolicy(Qt::WheelFocus);
}

Viewer::~Viewer()
{
}

void Viewer::enterEvent(QEvent *event)
{
    setFocus();
    event->accept();
}

void Viewer::leaveEvent(QEvent *event)
{
    clearFocus();
    event->accept();
}

//
// Handle mouse button down
//
void Viewer::mousePressEvent(QMouseEvent *event)
{
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);
    bool flag = false;

    // Create rubberBand
    if (rubberBand == NULL)
        rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

    // If left mouse button is pressed
    if (event->button() == Qt::LeftButton)
    {
        origin = event->pos();
        if (pasting)
        {
            pushImage();
        }
        else if ((leftMode == "Draw") || (leftMode == "Erase"))
        {
            pushImage();
            drawing = true;
            setCursor(Qt::CrossCursor);
        }
        else if ((leftMode == "Pointer") || (leftMode == "Fill"))
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
        if (shift)
        {
            setCursor(Qt::OpenHandCursor);
        }
        else
        {
            rubberBand->setGeometry(QRect(origin, QSize()));
            rubberBand->show();
        }
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
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);
    bool flag = false;

    if (pasting)
    {
        pasteLoc = event->pos();
        update();
        flag = true;
    }
    else
    {
        // If left mouse button is pressed
        if (event->buttons() & Qt::LeftButton)
        {
            if (drawing)
            {
                QColor color = (leftMode == "Draw") ? foregroundColor : backgroundColor;
                drawLine(origin, event->pos(), color);
                origin = event->pos();
            }
            else if ((leftMode == "Pointer") || (leftMode == "Fill"))
            {
                rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
            }
            flag = true;
        }
    }

    // If right mouse button pressed
    if (event->buttons() & Qt::RightButton)
    {
        if (shift)
        {
            QPoint delta = event->pos() - origin;
            scrollArea->horizontalScrollBar()->setValue(
                scrollArea->horizontalScrollBar()->value() - delta.x());
            scrollArea->verticalScrollBar()->setValue(
                scrollArea->verticalScrollBar()->value() - delta.y());
        }
        else
        {
            rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
        }
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
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);
    bool flag = false;

    // If left mouse button was released
    if (event->button() == Qt::LeftButton)
    {
        if (pasting)
        {
            pasteSelection();
            flag = true;
        }
        else if (drawing)
        {
            drawing = false;
            setCursor(Qt::ArrowCursor);
            flag = true;
        }
        else if (leftMode == "Fill")
        {
            rubberBand->hide();
            fillArea(rubberBand->geometry(), shift);
            flag = true;
        }
        if (flag)
        {
            currPage.setChanges(currPage.changes() + 1);
            currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit imageChangedSig();
        }
    }

    // If right mouse button was released
    if (event->button() == Qt::RightButton)
    {
        if (shift)
        {
            setCursor(Qt::ArrowCursor);
        }
        else
        {
            rubberBand->hide();
            zoomArea(rubberBand->geometry());
        }
        flag = true;
    }

    // Event was handled
    if (flag)
        event->accept();
    else
        QWidget::mouseReleaseEvent(event);
}

//
// Handle scroll wheel
//
void Viewer::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        zoomWheel(event->pos(), 1.25);
    else
        zoomWheel(event->pos(), 0.8);
    event->accept();
}

//
// Handle key press
//
void Viewer::keyPressEvent(QKeyEvent *event)
{
    bool flag = false;

    if (event->matches(QKeySequence::Copy))
    {
        copySelection();
        flag = true;
    }
    else if (event->matches(QKeySequence::Paste))
    {
        pasting = true;
        setMouseTracking(true);
        pasteLoc = mapFromGlobal(cursor().pos());
        update();
        flag = true;
    }
    else if (event->matches(QKeySequence::Undo))
    {
        undoEdit();
        flag = true;
    }
    else if (event->matches(QKeySequence::Redo))
    {
        redoEdit();
        flag = true;
    }
    else if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        if (event->key() == Qt::Key_X)
        {
            rubberBand->hide();
            fillArea(rubberBand->geometry(), false);
            flag = true;
        }
        else if (event->key() == Qt::Key_B)
        {
            rubberBand->hide();
            fillArea(rubberBand->geometry(), true);
            flag = true;
        }
        if (flag)
        {
            currPage.setChanges(currPage.changes() + 1);
            currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit imageChangedSig();
        }
    }

    // Event was handled
    if (flag)
        event->accept();
    else
        QWidget::keyPressEvent(event);
}

//
// Replace paintEvent to get proper scaling of image
//
void Viewer::paintEvent(QPaintEvent *)
{
    if (currPage.isNull() == false)
    {
        float scale = scaleBase * scaleFactor;
        QTransform transform = QTransform().scale(scale, scale);

        QPainter p(this);
        p.setTransform(transform);
        p.drawImage(currPage.rect().topLeft(), currPage);
        if (pasting)
        {
            qreal imgW = copyImage.size().width();
            qreal imgH = copyImage.size().width();
            QPointF loc = transform.inverted().map(pasteLoc) - QPointF(imgW/2.0, imgH/2.0);
            p.setOpacity(0.3);
            p.drawImage(loc, copyImage);
        }
        p.end();
    }
}

//
// Update viewer image from bookmarks
//
void Viewer::imageSelected(QListWidgetItem *curr, QListWidgetItem *)
{
    if (curr != NULL)
    {
        currPage = curr->data(Qt::UserRole).value<PageData>();
        if (currListItem != curr)
        {
            currListItem = curr;
            fitToWindow();
        }
    }
    else
    {
        currListItem = NULL;
        currPage = PageData();
    }

    // Turn off active operations
    if (rubberBand != NULL)
        rubberBand->hide();
    pasting = false;
    setMouseTracking(false);
    update();
}

//
// Always honor aspect ratio when resizing
//
QSize Viewer::sizeHint() const
{
    if (currPage.isNull())
        return parentWidget()->sizeHint();
    return currPage.size() * scaleBase * scaleFactor;
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
// Draw/Erase a line in the foreground/background color
//
void Viewer::drawLine(QPoint start, QPoint finish, QColor color)
{
    if (currPage.isNull())
        return;

    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();

    QPainter p(&currPage);
    p.setTransform(transform);
    qreal brush = int(brushSize * scale);
    p.setPen(QPen(color, brush, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(start, finish);
    p.end();
    update();
}

//
// Fill area with background color
//
void Viewer::fillArea(QRect rect, bool outside)
{
    if (currPage.isNull())
        return;

    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();

    pushImage();
    QPainter p(&currPage);
    if (outside)
    {
        QRect box = transform.mapRect(rect);
        int imgW = currPage.size().width();
        int imgH = currPage.size().height();

        p.fillRect(0, 0, imgW, box.top(), backgroundColor);
        p.fillRect(0, 0, box.left(), imgH, backgroundColor);
        p.fillRect(box.right(), 0, imgW, imgH, backgroundColor);
        p.fillRect(0, box.bottom(), imgW, imgH, backgroundColor);
    }
    else
    {
        p.setTransform(transform);
        p.fillRect(rect, backgroundColor);
    }
    p.end();
    update();
}

//
// Erase current page and insert 'Blank' into center
//
void Viewer::blankPage()
{
    if (currPage.isNull())
        return;

    pushImage();
    QPainter p(&currPage);
    p.fillRect(currPage.rect(), backgroundColor);
    p.setPen(foregroundColor);
    p.setFont(QFont("Courier", 20));
    p.drawText(currPage.rect(), Qt::AlignCenter, tr("BLANK"));
    p.end();
    currPage.setChanges(currPage.changes() + 1);
    currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit imageChangedSig();
    update();
}

//
// Copy selected region into local clipboard
//
void Viewer::copySelection()
{
    if (currPage.isNull())
        return;
    if ((rubberBand == NULL) || rubberBand->isHidden())
    {
        QMessageBox::information(this, "Copy", "Area must be selected");
        return;
    }

    // Get rectangle in image coordinates
    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();
    QRect box = transform.mapRect(rubberBand->geometry());

    copyImage = currPage.copy(box);
    rubberBand->hide();
}

//
// Paste from clipboard into page
//
void Viewer::pasteSelection()
{
    pasting = false;
    setMouseTracking(false);

    // Calculate position in image from pointer location
    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();
    qreal imgW = copyImage.size().width();
    qreal imgH = copyImage.size().width();
    QPointF loc = transform.map(pasteLoc) - QPointF(imgW/2.0, imgH/2.0);

    // Paint the copied section
    QPainter p(&currPage);
    p.drawImage(loc, copyImage);
    p.end();
    update();
}

//
// Save current image to undo buffer
//
void Viewer::pushImage()
{
    if (currPage.isNull())
        return;

    UndoBuffer ub = currListItem->data(Qt::UserRole+1).value<UndoBuffer>();
    ub.push(currPage);
    currListItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));
}

//
// Roll back one change
//
void Viewer::undoEdit()
{
    if (currPage.isNull())
        return;

    UndoBuffer ub = currListItem->data(Qt::UserRole+1).value<UndoBuffer>();
    currPage = ub.undo(currPage);
    currListItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));

    // Update listwidget with new image
    currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit imageChangedSig();
    update();
}

//
// Re-apply last change
//
void Viewer::redoEdit()
{
    if (currPage.isNull())
        return;

    UndoBuffer ub = currListItem->data(Qt::UserRole+1).value<UndoBuffer>();
    currPage = ub.redo(currPage);
    currListItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));

    // Update listwidget with new image
    currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit imageChangedSig();
    update();
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
    if (currPage.isNull())
        return;
    scaleFactor *= 1.25;
    updateGeometry();
    updateScrollBars();
    adjustScrollBars(1.25);
    emit zoomSig(scaleFactor);
}

//
// Draw image 20% smaller
void Viewer::zoomOut()
{
    if (currPage.isNull())
        return;
    scaleFactor *= 0.8;
    updateGeometry();
    updateScrollBars();
    adjustScrollBars(0.8);
    emit zoomSig(scaleFactor);
}

//
// Zoom to rubberband rectangle
//
void Viewer::zoomArea(QRect rect)
{
    if (currPage.isNull())
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
    if ((rectW < 5) || (rectH < 5))
        return;
    if ((rectW * viewH) > (rectH * viewW))
        scaleFactor *= (float)viewW / rectW;
    else
        scaleFactor *= (float)viewH / rectH;
    if (scaleFactor > 10000)
        scaleFactor = 10000;
    updateGeometry();
    updateScrollBars();

    // Adjust scrollbars so center of rect is center of viewport
    scrollArea->horizontalScrollBar()->setValue(int(
                centerX * scrollArea->horizontalScrollBar()->maximum() +
                ((centerX - 0.5) * scrollArea->horizontalScrollBar()->pageStep())));
    scrollArea->verticalScrollBar()->setValue(int(
                centerY * scrollArea->verticalScrollBar()->maximum() +
                ((centerY - 0.5) * scrollArea->verticalScrollBar()->pageStep())));
    emit zoomSig(scaleFactor);
}

//
// Zoom to wheel event
//
void Viewer::zoomWheel(QPoint pos, float factor)
{
    if (currPage.isNull())
        return;

    // Apply the zoom
    scaleFactor *= factor;
    updateGeometry();
    updateScrollBars();
    scrollArea->horizontalScrollBar()->setValue(int(
                scrollArea->horizontalScrollBar()->value() +
                (factor - 1) * pos.x()));
    scrollArea->verticalScrollBar()->setValue(int(
                scrollArea->verticalScrollBar()->value() +
                (factor - 1) * pos.y()));
    emit zoomSig(scaleFactor);
}

//
// Helper function for window fit functions
//     returns true if horizontal dimension is larger
//
bool Viewer::measureAll(PageData &page, int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH)
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
    imageW = page.size().width();
    imageH = page.size().height();

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

    if (currPage.isNull())
        return;

    // Scale to larger dimension
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        scaleBase = (float)viewH / imageH;
    else
        scaleBase = (float)viewW / imageW;

    // Update button actions
    scaleFactor = 1.0;
    emit zoomSig(scaleFactor);

    // Update scrollarea
    updateGeometry();
}

//
// Fit image to window with only a vertical scrollbar
//
void Viewer::fitWidth()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.isNull())
        return;

    // If height is larger dimension, leave space for vertical scroll bar
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        scaleBase = (float)(viewW - scrollBarSize) / imageW;
    else
        scaleBase = (float)viewW / imageW;

    // Update button actions
    scaleFactor = 1.0;
    emit zoomSig(scaleFactor);

    // Update scrollarea
    updateGeometry();
}

//
// Fit image to window with only a horizontal scrollbar
//
void Viewer::fitHeight()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.isNull())
        return;

    // If width is larger dimension, leave space for horizontal scroll bar
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        scaleBase = (float)(viewH - scrollBarSize) / imageH;
    else
        scaleBase = (float)viewH / imageH;

    // Update button actions
    scaleFactor = 1.0;
    emit zoomSig(scaleFactor);

    // Update scrollarea
    updateGeometry();
}

//
// Fit image to window with a maximum of one scrollbar
//
void Viewer::fillWindow()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.isNull())
        return;

    // Scale to smaller dimension
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        fitWidth();
    else
        fitHeight();
}
