#include "Viewer.h"
#include "UndoBuffer.h"
#include "QImage2OCV.h"
#include <QDebug>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

Viewer::Viewer(QWidget * parent) : QWidget(parent)
{
    setFocusPolicy(Qt::WheelFocus);

    // Setup logo
    logo = QImage(":/images/assets/tiffany.png");

    // Setup custom cursors
    QPixmap p;
    p = QPixmap(":/images/assets/pencil.svg").scaled(32,32,Qt::KeepAspectRatio);
    PencilCursor = QCursor(p, 0, 31);
    p = QPixmap(":/images/assets/dropper.svg").scaled(32,32,Qt::KeepAspectRatio);
    DropperCursor = QCursor(p, 0, 31);
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
    if (currPage.isNull())
        return;

    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);
    bool flag = false;

    // If left mouse button is pressed
    if (event->button() == Qt::LeftButton)
    {
        origin = event->pos();
        if (pasting)
        {
            pushImage();
        }
        else if (leftMode == Draw)
        {
            pushImage();
            setCursor(PencilCursor);
        }
        else if (leftMode == Select)
        {
            rubberBand->setGeometry(QRect(origin, QSize()));
            rubberBand->show();
        }
        else if (leftMode == ColorSelect)
        {
            setCursor(DropperCursor);
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
            rightMode = Pan;
        }
        else
        {
            rubberBand->setGeometry(QRect(origin, QSize()));
            rubberBand->show();
            rightMode = Zoom;
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
    if (currPage.isNull())
        return;

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
            if (leftMode == Draw)
            {
                drawLine(origin, event->pos(), foregroundColor);
                origin = event->pos();
            }
            else if (leftMode == Select)
            {
                rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
            }
            flag = true;
        }
    }

    // If right mouse button pressed
    if (event->buttons() & Qt::RightButton)
    {
        if (rightMode == Pan)
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
    if (currPage.isNull())
        return;

    bool flag = false;

    // If left mouse button was released
    if (event->button() == Qt::LeftButton)
    {
        if (pasting)
        {
            pasteSelection();
            flag = true;
        }
        else if (leftMode == Draw)
        {
            setCursor(Qt::ArrowCursor);
            flag = true;
        }
        else if (leftMode == ColorSelect)
        {
            colorSelect(event->pos());
            setCursor(Qt::ArrowCursor);
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
        if (rightMode == Pan)
        {
            setCursor(Qt::ArrowCursor);
        }
        else
        {
            rubberBand->hide();
            zoomArea(rubberBand->geometry());
        }
        rightMode = Idle;
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
    if (currPage.isNull())
        return;

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
    if (currPage.isNull())
        return;

    bool flag = false;

    if (event->key() == Qt::Key_Escape)
    {
        currMask = QImage();
        pasting = false;
        update();
        flag = true;
    }
    else if (event->matches(QKeySequence::Copy))
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
            if (currMask.isNull())
                fillArea(rubberBand->geometry(), false);
            else
            {
                applyMask(currMask, false);
                currMask = QImage();
            }
            flag = true;
        }
        else if (event->key() == Qt::Key_S)
        {
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
    QPainter p(this);

    if (!currPage.isNull())
    {
        float scale = scaleBase * scaleFactor;
        QTransform transform = QTransform().scale(scale, scale);

        p.setTransform(transform);
        p.drawImage(currPage.rect().topLeft(), currPage);
        if (pasting)
        {
            qreal imgW = copyImage.size().width();
            qreal imgH = copyImage.size().height();
            QPointF loc = transform.inverted().map(pasteLoc) - QPoint(imgW/2, imgH/2);
            p.setOpacity(0.3);
            p.drawImage(loc, copyImage);
        }
        else if (!currMask.isNull())
        {
            p.setOpacity(0.5);
            p.drawImage(QPoint(0,0), currMask);
        }
    }
    else
        p.drawImage((rect().bottomRight() - logo.rect().bottomRight())/2.0, logo);
    p.end();
}

//
// Update image selection from bookmarks
//
void Viewer::imageSelected(QListWidgetItem *curr, QListWidgetItem *)
{
    if (curr != NULL)
    {
        currListItem = curr;
        currPage = curr->data(Qt::UserRole).value<PageData>();
        fitToWindow();
    }
    else
    {
        currListItem = NULL;
        currPage = PageData();
    }
    updateViewer();
}

//
// Update viewer image from bookmarks
//
void Viewer::updateViewer()
{
    if (currPage.isNull())
        return;

    // Reload image from list
    currPage = currListItem->data(Qt::UserRole).value<PageData>();
    currMask = QImage();

    // Turn off active operations
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
        return logo.size();
    return currPage.size() * scaleBase * scaleFactor;
}

//
// Select pointer tool
//
void Viewer::pointerMode()
{
    leftMode = Select;
    currMask = QImage();
}

//
// Select color selection tool
//
void Viewer::dropperMode()
{
    leftMode = ColorSelect;
}

//
// Select draw line tool
//
void Viewer::pencilMode()
{
    leftMode = Draw;
    currMask = QImage();
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
// Draw a line in the foreground color
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
    if (rubberBand->isHidden())
    {
        QMessageBox::information(this, "Fill", "Area must be selected");
        return;
    }
    rubberBand->hide();

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
// slot to set dropper threshold
//
void Viewer::setThreshold(int val)
{
    dropperThreshold = val;
    currMask = QImage();
    update();
}

//
// Select all pixels near the cursor's color
//
void Viewer::colorSelect(QPoint pos)
{
    if (currPage.isNull())
        return;

    // Get pixel under cursor
    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();
    QPointF mPos = transform.map(QPointF(pos));
    QRgb pixel = currPage.pixel(mPos.x(), mPos.y());

    if (currPage.format() == QImage::Format_RGB32)
    {
        int red = qRed(pixel);
        int grn = qGreen(pixel);
        int blu = qBlue(pixel);
        qInfo() << red << grn << blu;

        // Identically sized grayscale image
        currMask = QImage(currPage.size(), QImage::Format_ARGB32);

        // Scan through page seeking matches
        QRgb blank = qRgba(255,255,255,255);// Don't match white pixels
        QRgb white = qRgba(255,255,255,0);  // Transparent white
        QRgb black = qRgba(0,0,0,255);      // Opaque black
        for(int i=0; i<currPage.height(); i++)
        {
            QRgb *srcPtr = (QRgb *)currPage.scanLine(i);
            QRgb *maskPtr = (QRgb *)currMask.scanLine(i);
            for(int j=0; j<currPage.width(); j++)
            {
                QRgb val = *srcPtr++;
                int max = abs(red - qRed(val));
                int tmp = abs(grn - qGreen(val));
                if (tmp > max)
                    max = tmp;
                tmp = abs(blu - qBlue(val));
                if (tmp > max)
                    max = tmp;
                *maskPtr++ = (val == blank) || (max > dropperThreshold) ? white : black;
            }
        }
    }
    else if (currPage.format() == QImage::Format_Grayscale8)
    {
        int pix = qRed(pixel);

        // Identically sized grayscale image
        currMask = QImage(currPage.size(), QImage::Format_ARGB32);

        // Scan through page seeking matches
        QRgb white = qRgba(255,255,255,0);  // Transparent white
        QRgb black = qRgba(0,0,0,255);      // Opaque black
        for(int i=0; i<currPage.height(); i++)
        {
            uchar *srcPtr = currPage.scanLine(i);
            QRgb *maskPtr = (QRgb *)currMask.scanLine(i);
            for(int j=0; j<currPage.width(); j++)
            {
                int val = *srcPtr++;
                int max = abs(pix - val);
                *maskPtr++ = (val == 255) || (max > dropperThreshold) ? white : black;
            }
        }
    }
    else
    {
        QMessageBox::information(this, "colorSelect", "Only works on RGB and grayscale images");
        return;
    }
    update();
}

//
// Apply the mask to the current image
//
void Viewer::applyMask(QImage &mask, bool flag)
{
    // Paint the copied section
    pushImage();
    QPainter p(&currPage);
    if (!flag)
        mask.invertPixels(QImage::InvertRgb);
    p.drawImage(QPoint(0,0), mask);
    p.end();
    update();
}

//
// Copy selected region into local clipboard
//
void Viewer::copySelection()
{
    if (currPage.isNull())
        return;
    if (rubberBand->isHidden())
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
    qreal imgH = copyImage.size().height();
    QPointF loc = transform.map(pasteLoc) - QPoint(imgW/2, imgH/2);

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

    // Save for later
    int hVal = scrollArea->horizontalScrollBar()->value();
    int vVal = scrollArea->verticalScrollBar()->value();

    // Apply the zoom
    scaleFactor *= factor;
    updateGeometry();
    updateScrollBars();
    scrollArea->horizontalScrollBar()->setValue(int(hVal + (factor - 1) * pos.x()));
    scrollArea->verticalScrollBar()->setValue(int(vVal + (factor - 1) * pos.y()));
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
