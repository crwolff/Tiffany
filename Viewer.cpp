#include "Viewer.h"
#include "UndoBuffer.h"
#include "ViewData.h"
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
    p = QPixmap(":/images/assets/pencil180.svg").scaled(32,32,Qt::KeepAspectRatio);
    Pencil180Cursor = QCursor(p, 31, 0);
    p = QPixmap(":/images/assets/dropper.svg").scaled(32,32,Qt::KeepAspectRatio);
    DropperCursor = QCursor(p, 0, 31);
    p = QPixmap(":/images/assets/despeckle.svg").scaled(32,32,Qt::KeepAspectRatio);
    DespeckleCursor = QCursor(p, 15, 15);
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
        else if ((leftMode == Pencil) || (leftMode == Eraser))
        {
            currColor = (leftMode == Pencil) ? foregroundColor : backgroundColor;
            pushImage();
            setCursor(pencil180 ? Pencil180Cursor : PencilCursor);
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
        else if (leftMode == Despeckle)
        {
            setCursor(DespeckleCursor);
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

    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);
    bool ctrl = keyMod.testFlag(Qt::ControlModifier);
    bool flag = false;

    if (pasting)
    {
        pasteLoc = event->pos();
        pasteCtrl = ctrl;
        update();
        flag = true;
    }
    else
    {
        // If left mouse button is pressed
        if (event->buttons() & Qt::LeftButton)
        {
            if ((leftMode == Pencil) || (leftMode == Eraser))
            {
                if (shift)
                {
                    shiftPencil = true;
                    drawLoc = event->pos();
                    update();
                }
                else
                {
                    shiftPencil = false;
                    drawLine(origin, event->pos(), currColor);
                    origin = event->pos();
                }
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
            pasteSelection(pasteCtrl);
            flag = true;
        }
        else if ((leftMode == Pencil) || (leftMode == Eraser))
        {
            shiftPencil = false;
            drawLine(origin, event->pos(), currColor);
            setCursor(Qt::ArrowCursor);
            flag = true;
        }
        else if (leftMode == ColorSelect)
        {
            float scale = scaleBase * scaleFactor;
            QTransform transform = QTransform().scale(scale, scale).inverted();
            dropperLoc = transform.map(QPointF(event->pos()));
            colorSelect();
            setCursor(Qt::ArrowCursor);
        }
        else if (leftMode == FloodFill)
        {
            float scale = scaleBase * scaleFactor;
            QTransform transform = QTransform().scale(scale, scale).inverted();
            floodLoc = transform.map(QPointF(event->pos()));
            floodFill();
        }
        else if (leftMode == Despeckle)
        {
            despeckle();
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
        deskewImg = QImage();
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
        if (!deskewImg.isNull())
        {
            applyDeskew();
            deskewImg = QImage();
            update();
            currPage.setChanges(currPage.changes() + 1);
            currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit imageChangedSig();
            flag = true;
        }
        else if (!copyImage.isNull())
        {
            if (pasting)
            {
                int idx = copyImageList.indexOf(copyImage);
                if ((idx + 1) < copyImageList.size())
                    copyImage = copyImageList.at(idx+1);
                else
                    copyImage = copyImageList.at(0);
            }
            pasting = true;
            setMouseTracking(true);
            pasteLoc = mapFromGlobal(cursor().pos());
            update();
            flag = true;
        }
    }
    else if (event->matches(QKeySequence::Delete))
    {
        if (pasting)
        {
            int idx = copyImageList.indexOf(copyImage);
            if (idx >= 0)
                copyImageList.removeAt(idx);
            if (copyImageList.size() > 0)
            {
                copyImage = copyImageList.at(0);
            }
            else
            {
                copyImage = QImage();
                pasting = false;
            }
            update();
            flag = true;
        }
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
                blinkTimer->stop();
                applyMask(currMask, false);
                currMask = QImage();
            }
            flag = true;
        }
        else if (event->key() == Qt::Key_S)
        {
            if (currMask.isNull())
                fillArea(rubberBand->geometry(), true);
            else
            {
                blinkTimer->stop();
                applyMask(currMask, true);
                currMask = QImage();
            }
            flag = true;
        }
        if (flag)
        {
            currPage.setChanges(currPage.changes() + 1);
            currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit imageChangedSig();
        }
    }
    else if (event->modifiers().testFlag(Qt::ShiftModifier) && !deskewImg.isNull())
    {
        if (event->key() == Qt::Key_Down)
        {
            gridOffsetY++;
            while (gridOffsetY > 50)
                gridOffsetY -= 50;
            update();
            flag = true;
        }
        else if (event->key() == Qt::Key_Up)
        {
            gridOffsetY--;
            while (gridOffsetY < 0)
                gridOffsetY += 50;
            update();
            flag = true;
        }
        else if (event->key() == Qt::Key_Right)
        {
            gridOffsetX++;
            while (gridOffsetX > 50)
                gridOffsetX -= 50;
            update();
            flag = true;
        }
        else if (event->key() == Qt::Key_Left)
        {
            gridOffsetX--;
            while (gridOffsetX < 0)
                gridOffsetX += 50;
            update();
            flag = true;
        }
    }
    else // Not control or shift
    {
        if (event->key() == Qt::Key_F)
        {
            fitToWindow();
            flag = true;
        }
        else if (event->key() == Qt::Key_T)
        {
            pencil180 = !pencil180;
            if ((cursor() == PencilCursor) || (cursor() == Pencil180Cursor))
                setCursor(pencil180 ? Pencil180Cursor : PencilCursor);
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
            if (pasteCtrl)
                loc = pasteOptimizer(imgW, imgH, loc);
            p.setOpacity(0.3);
            p.drawImage(loc, copyImage);
        }
        else if (!currMask.isNull())
        {
            QImage tmp = currMask;
            if (blink)
                tmp.invertPixels(QImage::InvertRgb);
            p.drawImage(QPoint(0,0), tmp);
        }
        else if (!deskewImg.isNull())
        {
            // Draw deskewed image rotated about the center
            QRect rect(deskewImg.rect());
            rect.moveCenter(currPage.rect().center());
            p.drawImage(rect.topLeft(), deskewImg);

            // Draw alignment grid
            p.setTransform(QTransform());           // Reset to view coordinates
            p.setOpacity(0.5);
            for(int idx=gridOffsetX + (width() % 50)/2; idx < width(); idx += 50)
                p.drawLine(idx, 0, idx, height());
            for(int idx=gridOffsetY + (height() % 50)/2; idx < height(); idx += 50)
                p.drawLine(0, idx, width(), idx);
        }
        else if (shiftPencil)
        {
            QPointF start = transform.inverted().map(origin);
            QPointF finish = transform.inverted().map(drawLoc);
            p.setPen(QPen(currColor, brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(start, finish);
        }
    }
    else
        p.drawImage((rect().bottomRight() - logo.rect().bottomRight())/2.0, logo);
    p.end();
}

//
// Blink mask to make it easier to see
//
void Viewer::blinker()
{
    if (currPage.isNull() || currMask.isNull())
    {
        blinkTimer->stop();
        return;
    }
    blink = !blink;
    update();
}

//
// Update image selection from bookmarks
//
void Viewer::imageSelected(QListWidgetItem *curr, QListWidgetItem *)
{
    // Save current view
    if (currListItem != nullptr)
    {
        ViewData v = currListItem->data(Qt::UserRole+2).value<ViewData>();
        v.scaleBase = scaleBase;
        v.scaleFactor = scaleFactor;
        v.horizontalScroll = scrollArea->horizontalScrollBar()->value();
        v.verticalScroll = scrollArea->verticalScrollBar()->value();
        currListItem->setData(Qt::UserRole+2, QVariant::fromValue(v));
    }

    // Load new page
    if (curr != NULL)
    {
        currListItem = curr;
        currPage = curr->data(Qt::UserRole).value<PageData>();
        ViewData v = curr->data(Qt::UserRole+2).value<ViewData>();
        if (v.scaleBase != 0.0)
        {
            scaleBase = v.scaleBase;
            scaleFactor = v.scaleFactor;
            updateGeometry();
            updateScrollBars();
            scrollArea->horizontalScrollBar()->setValue(v.horizontalScroll);
            scrollArea->verticalScrollBar()->setValue(v.verticalScroll);
            emit zoomSig(scaleFactor);
        }
        else
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
    deskewImg = QImage();

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
    deskewImg = QImage();
}

//
// Select color selection tool
//
void Viewer::dropperMode()
{
    leftMode = ColorSelect;
    currMask = QImage();
    deskewImg = QImage();
}

//
// Select flood fill tool
//
void Viewer::floodMode()
{
    leftMode = FloodFill;
    currMask = QImage();
    deskewImg = QImage();
}

//
// Select draw line tool
//
void Viewer::pencilMode()
{
    leftMode = Pencil;
    pencil180 = false;
    currMask = QImage();
    deskewImg = QImage();
}

//
// Select erase line tool
//
void Viewer::eraserMode()
{
    leftMode = Eraser;
    pencil180 = false;
    currMask = QImage();
    deskewImg = QImage();
}

//
// Select deskew mode
//
void Viewer::deskewMode()
{
    leftMode = Deskew;
    currMask = QImage();
    deskewImg = QImage();
    deskew();
    gridOffsetX = 0;
    gridOffsetY = 0;
}

//
// Select despeckle mode
//
void Viewer::despeckleMode()
{
    leftMode = Despeckle;
    currMask = QImage();
    deskewImg = QImage();
    despeckle();
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
    p.setPen(QPen(color, int(brushSize * scale), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
void Viewer::setDropperThreshold(int val)
{
    dropperThreshold = val;
    if (leftMode == ColorSelect)
        colorSelect();
}

//
// Select all pixels near the cursor's color
//
void Viewer::colorSelect()
{
    if (currPage.isNull())
        return;

    // Get pixel under cursor
    QRgb pixel = currPage.pixel(dropperLoc.x(), dropperLoc.y());

    if (currPage.format() == QImage::Format_RGB32)
    {
        int red = qRed(pixel);
        int grn = qGreen(pixel);
        int blu = qBlue(pixel);

        // Identically sized grayscale image
        currMask = QImage(currPage.size(), QImage::Format_ARGB32);
        deskewImg = QImage();

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
        blink = false;
        blinkTimer->start(500);
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
        blink = false;
        blinkTimer->start(500);
    }
    else
    {
        QMessageBox::information(this, "colorSelect", "Only works on RGB and grayscale images");
        return;
    }
    update();
}

//
// slot to set flood fill threshold
//
void Viewer::setFloodThreshold(int val)
{
    floodThreshold = val;
    if (leftMode == FloodFill)
        floodFill();
}

//
// Select all pixels near the cursor's color
//
void Viewer::floodFill()
{
    if (currPage.isNull())
        return;

    if (currPage.format() == QImage::Format_Mono)
    {
        QMessageBox::information(this, "floodFill", "Only works on RGB and grayscale images");
        return;
    }

    // Convert to OpenCV
    cv::Mat orig = QImage2OCV(currPage);
    if (currPage.format() == QImage::Format_RGB32)
        cv::cvtColor(orig, orig, cv::COLOR_RGBA2RGB);   // floodfill doesn't work with alpha channel

    // Make all zero mask
    cv::Mat floodMask = cv::Mat::zeros(currPage.height() + 2, currPage.width() + 2, CV_8UC1);

    // Fill adjacent pixels
    cv::Point loc(floodLoc.x(), floodLoc.y());
    cv::Rect region;
    cv::Scalar thresh(floodThreshold, floodThreshold, floodThreshold);
    int flags = 8 | (255 << 8 ) | cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY;
    cv::floodFill(orig, floodMask, loc, 0, &region, thresh, thresh, flags);

    // Convert mask back
    QImage tmp = OCV2QImage(floodMask);
    tmp = tmp.copy(1, 1, tmp.width() - 2, tmp.height() - 2);

    // Identically sized image
    currMask = QImage(currPage.size(), QImage::Format_ARGB32);

    // Scan through page seeking matches
    QRgb white = qRgba(255,255,255,0);  // Transparent white
    QRgb black = qRgba(0,0,0,255);      // Opaque black
    for(int i=0; i<tmp.height(); i++)
    {
        uchar *srcPtr = tmp.scanLine(i);
        QRgb *maskPtr = (QRgb *)currMask.scanLine(i);
        for(int j=0; j<tmp.width(); j++)
            *maskPtr++ = (*srcPtr++ > 128) ? black : white;
    }

    // Highlight changes
    blink = false;
    blinkTimer->start(500);

    // Repaint
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
// Set deskew angle
//
void Viewer::setDeskew(double angle)
{
    deskewAngle = angle;
    if (leftMode == Deskew)
        deskew();
}

//
// Deskew image by small amount
//
void Viewer::deskew()
{
    if (currPage.isNull())
        return;

    // Identically sized image
    deskewImg = QImage(currPage.size(), currPage.format());
    currMask = QImage();

    // Rotate page
    QTransform tmat = QTransform().rotate(deskewAngle);
    deskewImg = currPage.transformed(tmat, Qt::SmoothTransformation);
    update();
}

//
// Apply the deskewImg to the current page
//
void Viewer::applyDeskew()
{
    // Paint the deskew image rotated about the center
    // Note: This code is identical to paintEvent
    pushImage();
    QPainter p(&currPage);
    QRect rect(deskewImg.rect());
    rect.moveCenter(currPage.rect().center());
    p.drawImage(rect.topLeft(), deskewImg);
    p.end();
}

//
// Slot to set despeckle area
//
void Viewer::setDespeckle(int val)
{
    despeckleArea = val;
    if (leftMode == Despeckle)
        despeckle();
}

//
// Calculate the despeckle mask
//
void Viewer::despeckle()
{
    if (currPage.isNull())
        return;

    // Convert to grayscale
    QImage img;
    if ((currPage.format() == QImage::Format_RGB32) || (currPage.format() == QImage::Format_Mono))
        img = currPage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
    else
        img = currPage;

    // Convert to OpenCV
    cv::Mat mat = QImage2OCV(img);

    // Make B&W with background black
    cv::Mat bw;
    //cv::threshold(mat, bw, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    cv::threshold(mat, bw, 250, 255, cv::THRESH_BINARY_INV);

    // Find blobs
    cv::Mat stats, centroids, labelImg;
    int nLabels = cv::connectedComponentsWithStats(bw, labelImg, stats, centroids, 4, CV_32S);

    // Build mask of blobs smaller than limit
    cv::Vec4b white = cv::Vec4b(255,255,255,0);  // Transparent white
    cv::Vec4b black = cv::Vec4b(0,0,0,255);      // Opaque black
    cv::Mat mask(labelImg.size(), CV_8UC4, white);
    int cnt = 0;
    for(int idx=1; idx<nLabels; idx++)
    {
        // Check if this blob is small enough
        if (stats.at<int>(idx, cv::CC_STAT_AREA) <= despeckleArea)
        {
            int top = stats.at<int>(idx, cv::CC_STAT_TOP);
            int bot = stats.at<int>(idx, cv::CC_STAT_TOP) + stats.at<int>(idx, cv::CC_STAT_HEIGHT);
            int left = stats.at<int>(idx, cv::CC_STAT_LEFT);
            int right = stats.at<int>(idx, cv::CC_STAT_LEFT) + stats.at<int>(idx, cv::CC_STAT_WIDTH);

            // Sweep the enclosing rectangle, setting pixels in the mask
            for (int row=top; row<bot; row++)
            {
                for (int col=left; col<right; col++)
                {
                    if (labelImg.at<int>(row,col) == idx)
                    {
                        cv::Vec4b &pixel = mask.at<cv::Vec4b>(row,col);
                        pixel = black;
                    }
                }
            }
            cnt++;
        }
    }
    //qInfo() << cnt << "Blobs detected";
    currMask = OCV2QImage(mask);

    // Blink mask
    blink = false;
    blinkTimer->start(500);
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
    copyImageList.prepend(copyImage);
    if (copyImageList.size() > 12)
        copyImageList.removeLast();
    rubberBand->hide();
}

//
// Find best location for pasting based on correlation
//
QPointF Viewer::pasteOptimizer(qreal &imgW, qreal &imgH, QPointF &loc)
{
    int win = 4;

    // Copy a region slightly larger than the paste
    QImage tmp1 = currPage.copy(loc.x() - win, loc.y() - win, imgW + win*2, imgH + win*2);

    // Convert color images to grayscale first
    if (tmp1.format() != QImage::Format_Grayscale8)
        tmp1 = tmp1.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat1 = QImage2OCV(tmp1);

    // Copy the paste image
    QImage tmp2 = copyImage;

    // Convert color images to grayscale first
    if (tmp2.format() != QImage::Format_Grayscale8)
        tmp2 = tmp2.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat2 = QImage2OCV(tmp2);

    // Make a target array
    cv::Mat res;
    res.create( win * 2 + 1, win * 2 + 1, CV_32FC1 );

    // Correlate the images
    cv::matchTemplate( mat1, mat2, res, cv::TM_CCOEFF );

    // Find max
    cv::Point matchLoc;
    cv::minMaxLoc( res, NULL, NULL, NULL, &matchLoc, cv::Mat() );

    // Snap to best location
    return QPointF( loc.x() + matchLoc.x - win, loc.y() + matchLoc.y - win );
}

//
// Paste from clipboard into page
//
void Viewer::pasteSelection(bool ctrl)
{
    pasting = false;
    setMouseTracking(false);

    // Move this copyImage to head of list (last to get dropped)
    int idx = copyImageList.indexOf(copyImage);
    if (idx > 0)
        copyImageList.move(idx, 0);

    // Calculate position in image from pointer location
    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();
    qreal imgW = copyImage.size().width();
    qreal imgH = copyImage.size().height();
    QPointF loc = transform.map(pasteLoc) - QPoint(imgW/2, imgH/2);

    // If control is pressed, snap to 'best' location
    if (ctrl)
        loc = pasteOptimizer(imgW, imgH, loc);

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
