#include "Config.h"
#include "Viewer.h"
#include "UndoBuffer.h"
#include "ViewData.h"
#include "QImage2OCV.h"
#include <QApplication>
#include <QDebug>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>

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
    if (tessApi != nullptr)
    {
        tessApi->End();
        delete tessApi;
    }
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
            pushImage(currListItem, currPage);
        }
        else if ((leftMode == Pencil) || (leftMode == Eraser))
        {
            currColor = (leftMode == Pencil) ? foregroundColor : backgroundColor;
            pushImage(currListItem, currPage);
            setCursor(pencil180 ? Pencil180Cursor : PencilCursor);
            drawDot(origin, currColor);
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
        else if ((leftMode == Despeckle) || (leftMode == Devoid))
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
            LastCursor = cursor();
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

    // Marking corners for warping
    if (warpCount > 0)
    {
        drawLoc = event->pos();
        update();
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
        else if (leftMode == Devoid)
        {
            devoid();
            setCursor(Qt::ArrowCursor);
        }
        else if (warpCount > 0)
        {
            float scale = scaleBase * scaleFactor;
            QTransform transform = QTransform().scale(scale, scale).inverted();
            QPointF tmp = transform.map(QPointF(event->pos()));
            warpCorner[warpCount-1].x = tmp.x();
            warpCorner[warpCount-1].y = tmp.y();
            warpCount++;
            if (warpCount > 4)
            {
                warpCount = 0;
                setMouseTracking(false);
                pushImage(currListItem, currPage);
                doWarp();
                setCursor(Qt::ArrowCursor);
            }
        }
        else if (leftMode == LocateRef)
        {
            float scale = scaleBase * scaleFactor;
            QTransform transform = QTransform().scale(scale, scale).inverted();
            QPointF tmp = transform.map(QPointF(event->pos()));
            if (shiftLocate)
                Config::locate2 = tmp;
            else
                Config::locate1 = tmp;
            setCursor(Qt::ArrowCursor);
            leftMode = Select;
            update();
        }
        else if (leftMode == PlaceRef)
        {
            float scale = scaleBase * scaleFactor;
            QTransform transform = QTransform().scale(scale, scale).inverted();
            QPointF tmp = transform.map(QPointF(event->pos()));
            if (shiftLocate)
                tmp = Config::locate2 - tmp;
            else
                tmp = Config::locate1 - tmp;
            pushImage(currListItem, currPage);
            QImage xxx = currPage;
            QPainter p(&currPage);
            p.fillRect(currPage.rect(), backgroundColor);
            p.drawImage(tmp, xxx);
            p.end();
            flag = true;
        }
        if (flag)
        {
            currPage.setChanges(currPage.changes() + 1);
            currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit imageChangedSig();
            update();
        }
    }

    // If right mouse button was released
    if (event->button() == Qt::RightButton)
    {
        if (rightMode == Pan)
        {
            setCursor(LastCursor);
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
        fgMask = QImage();
        bgMask = QImage();
        emit statusSig("");
        deskewImg = QImage();
        pasting = false;
        warpCount = 0;
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
            if (fgMask.isNull() || bgMask.isNull())
                fillArea(rubberBand->geometry(), false);
            else
            {
                blinkTimer->stop();
                if (leftMode == Devoid)
                    applyMask(fgMask);
                else
                    applyMask(bgMask);
                fgMask = QImage();
                bgMask = QImage();
            }
            flag = true;
        }
        else if (event->key() == Qt::Key_S)
        {
            if (fgMask.isNull() || bgMask.isNull())
            {
                if (event->modifiers().testFlag(Qt::ShiftModifier))
                    cropArea(rubberBand->geometry());
                else
                    fillArea(rubberBand->geometry(), true);
            }
            else
            {
                blinkTimer->stop();
                if (leftMode == Devoid)
                    applyMask(bgMask);
                else
                    applyMask(fgMask);
                fgMask = QImage();
                bgMask = QImage();
            }
            flag = true;
        }
        if (flag)
        {
            currPage.setChanges(currPage.changes() + 1);
            currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit imageChangedSig();
        }

        if (event->key() == Qt::Key_T)
        {
            regionOCR();
            flag = true;
        }
        else if (event->key() == Qt::Key_G)
        {
            calcDeskew();
            flag = true;
        }
        else if (event->key() == Qt::Key_W)
        {
            leftMode = Select;
            blinkTimer->stop();
            fgMask = QImage();
            bgMask = QImage();
            emit statusSig("");
            deskewImg = QImage();
            setCursor(Qt::CrossCursor);
            warpCount = 1;
            setMouseTracking(true);
            flag = true;
        }
        else if (event->key() == Qt::Key_R)
        {
            if (event->modifiers().testFlag(Qt::ShiftModifier))
                shiftLocate = true;
            else
                shiftLocate = false;
            leftMode = LocateRef;
            blinkTimer->stop();
            fgMask = QImage();
            bgMask = QImage();
            emit statusSig("");
            deskewImg = QImage();
            setCursor(Qt::CrossCursor);
            update();
            flag = true;
        }
        else if (event->key() == Qt::Key_E)
        {
            if (event->modifiers().testFlag(Qt::ShiftModifier))
                shiftLocate = true;
            else
                shiftLocate = false;
            leftMode = PlaceRef;
            blinkTimer->stop();
            fgMask = QImage();
            bgMask = QImage();
            emit statusSig("");
            deskewImg = QImage();
            setCursor(Qt::CrossCursor);
            flag = true;
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
        else if (!fgMask.isNull() && !bgMask.isNull())
        {
            p.drawImage(QPoint(0,0), blink ? fgMask : bgMask);
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
            p.setPen(QPen(currColor, Config::brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(start, finish);
        }
        else if (warpCount > 1)
        {
            p.setPen(QPen(Qt::green, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            QPointF start = transform.inverted().map(drawLoc);
            for(int idx=0;idx < warpCount-1;idx++)
            {
                QPointF finish = QPointF(warpCorner[idx].x, warpCorner[idx].y);
                p.drawLine(start, finish);
            }
        }
        else if (leftMode == LocateRef)
        {
            // Draw alignment grid
            p.setTransform(QTransform());           // Reset to view coordinates
            p.setOpacity(0.5);
            p.drawLine(width()/2, 0, width()/2, height());
            for(int idx=(width() % 50)/2; idx < width(); idx += 50)
                p.drawLine(idx, 0, idx, height());
            for(int idx=(height() % 50)/2; idx < height(); idx += 50)
                p.drawLine(0, idx, width(), idx);
            p.setPen(Qt::red);
            p.drawLine(width()/2, 0, width()/2, height());
            p.drawLine(0, height()/2, width(), height()/2);
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
    if (currPage.isNull() || fgMask.isNull() || bgMask.isNull())
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
        v.deskewAngle = Config::deskewAngle;
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
        Config::deskewAngle = v.deskewAngle;
        emit setDeskewWidget(Config::deskewAngle);
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
    fgMask = QImage();
    bgMask = QImage();
    emit statusSig("");
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
void Viewer::setTool(LeftMode tool)
{
    setCursor(Qt::ArrowCursor);
    leftMode = tool;
    fgMask = QImage();
    bgMask = QImage();
    emit statusSig("");
    deskewImg = QImage();
    if ((tool == Pencil) || (tool == Eraser))
        pencil180 = false;
    else if (tool == Deskew)
    {
        deskew();
        gridOffsetX = 0;
        gridOffsetY = 0;
    }
    else if (tool == Despeckle)
        despeckle();
    else if (tool == Devoid)
        devoid();
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
    p.setPen(QPen(color, int(Config::brushSize * scale), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(start, finish);
    p.end();
    update();
}

//
// Draw a dot in the foreground color
//
void Viewer::drawDot(QPoint loc, QColor color)
{
    if (currPage.isNull())
        return;

    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();

    QPainter p(&currPage);
    p.setTransform(transform);
    p.setBrush(color);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(color, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawEllipse(loc, int(Config::brushSize * scale/2.0), int(Config::brushSize * scale/2.0));
    p.end();
    update();
}

//
// Crop to rectangle
//
void Viewer::cropArea(QRect rect)
{
    if (currPage.isNull())
        return;
    if (rubberBand->isHidden())
    {
        QMessageBox::information(this, "Crop", "Area must be selected");
        return;
    }
    rubberBand->hide();

    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();

    pushImage(currListItem, currPage);
    QRect box = transform.mapRect(rect);
    PageData newPage = currPage.copy(box);
    newPage.copyOtherData(currPage);
    currPage = newPage;
    fitToWindow();
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

    pushImage(currListItem, currPage);
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

    bool ok;
    QString text = QInputDialog::getText(this, tr("New Page Contents"),
                                         "", QLineEdit::Normal,
                                         "BLANK", &ok);
    if (ok)
    {
        pushImage(currListItem, currPage);
        QPainter p(&currPage);
        p.fillRect(currPage.rect(), backgroundColor);
        p.setPen(foregroundColor);
        p.setFont(Config::textFont);
        p.drawText(currPage.rect(), Qt::AlignCenter, text);
        p.end();
        currPage.setChanges(currPage.changes() + 1);
        currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
        emit imageChangedSig();
        update();
    }
}

//
// slot to set dropper threshold
//
void Viewer::setDropperThreshold(int val)
{
    Config::dropperThreshold = val;
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
    deskewImg = QImage();

    // Get pixel under cursor
    QRgb pixel = currPage.pixel(dropperLoc.x(), dropperLoc.y());

    if ((currPage.format() == QImage::Format_RGB32) || (currPage.format() == QImage::Format_ARGB32))
    {
        blinkTimer->stop();
        int red = qRed(pixel);
        int grn = qGreen(pixel);
        int blu = qBlue(pixel);

        // Masks to paint on
        fgMask = QImage(currPage.size(), QImage::Format_ARGB32);
        bgMask = QImage(currPage.size(), QImage::Format_ARGB32);

        // Scan through page seeking matches
        QRgb transparent = qRgba(0,0,0,0);
        for(int i=0; i<currPage.height(); i++)
        {
            QRgb *srcPtr = (QRgb *)currPage.scanLine(i);
            QRgb *fgMaskPtr = reinterpret_cast<QRgb*>(fgMask.scanLine(i));
            QRgb *bgMaskPtr = reinterpret_cast<QRgb*>(bgMask.scanLine(i));
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
                if (max > Config::dropperThreshold)
                {
                    *fgMaskPtr++ = transparent;
                    *bgMaskPtr++ = transparent;
                }
                else
                {
                    *fgMaskPtr++ = foregroundColor.rgb();
                    *bgMaskPtr++ = backgroundColor.rgb();
                }
            }
        }
        blink = false;
        blinkTimer->start(500);
    }
    else if (currPage.format() == QImage::Format_Grayscale8)
    {
        blinkTimer->stop();
        int pix = qRed(pixel);

        // Masks to paint on
        fgMask = QImage(currPage.size(), QImage::Format_ARGB32);
        bgMask = QImage(currPage.size(), QImage::Format_ARGB32);

        // Scan through page seeking matches
        QRgb transparent = qRgba(0,0,0,0);
        for(int i=0; i<currPage.height(); i++)
        {
            uchar *srcPtr = currPage.scanLine(i);
            QRgb *fgMaskPtr = reinterpret_cast<QRgb*>(fgMask.scanLine(i));
            QRgb *bgMaskPtr = reinterpret_cast<QRgb*>(bgMask.scanLine(i));
            for(int j=0; j<currPage.width(); j++)
            {
                int val = *srcPtr++;
                int max = abs(pix - val);
                if (max > Config::dropperThreshold)
                {
                    *fgMaskPtr++ = transparent;
                    *bgMaskPtr++ = transparent;
                }
                else
                {
                    *fgMaskPtr++ = foregroundColor.rgb();
                    *bgMaskPtr++ = backgroundColor.rgb();
                }
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
    Config::floodThreshold = val;
    if (leftMode == FloodFill)
        floodFill();
}

//
// Select all pixels near the cursor's color
//
void Viewer::floodFill()
{
    blinkTimer->stop();
    if (currPage.isNull())
        return;

    // Upconvert mono images
    QImage tmpPage;
    if (currPage.format() == QImage::Format_Mono)
        tmpPage = currPage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
    else
        tmpPage = currPage;

    // Convert to OpenCV
    cv::Mat orig = QImage2OCV(tmpPage);
    if ((tmpPage.format() == QImage::Format_RGB32) || (tmpPage.format() == QImage::Format_ARGB32))
        cv::cvtColor(orig, orig, cv::COLOR_RGBA2RGB);   // floodfill doesn't work with alpha channel

    // Make all zero mask
    cv::Mat floodMask = cv::Mat::zeros(tmpPage.height() + 2, tmpPage.width() + 2, CV_8UC1);

    // Fill adjacent pixels
    cv::Point loc(floodLoc.x(), floodLoc.y());
    cv::Rect region;
    cv::Scalar thresh(Config::floodThreshold, Config::floodThreshold, Config::floodThreshold);
    int flags = 8 | (255 << 8 ) | cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY;
    cv::floodFill(orig, floodMask, loc, 0, &region, thresh, thresh, flags);

    // Convert mask back
    QImage tmp = OCV2QImage(floodMask);
    tmp = tmp.copy(1, 1, tmp.width() - 2, tmp.height() - 2);

    // Masks to paint on
    fgMask = QImage(currPage.size(), QImage::Format_ARGB32);
    bgMask = QImage(currPage.size(), QImage::Format_ARGB32);

    // Scan through page seeking matches
    QRgb transparent = qRgba(0,0,0,0);
    for(int i=0; i<tmp.height(); i++)
    {
        uchar *srcPtr = tmp.scanLine(i);
        QRgb *fgMaskPtr = reinterpret_cast<QRgb*>(fgMask.scanLine(i));
        QRgb *bgMaskPtr = reinterpret_cast<QRgb*>(bgMask.scanLine(i));
        for(int j=0; j<tmp.width(); j++)
        {
            if (*srcPtr++ > 128)
            {
                *fgMaskPtr++ = foregroundColor.rgb();
                *bgMaskPtr++ = backgroundColor.rgb();
            }
            else
            {
                *fgMaskPtr++ = transparent;
                *bgMaskPtr++ = transparent;
            }
        }
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
void Viewer::applyMask(QImage &mask)
{
    // Paint the copied section
    pushImage(currListItem, currPage);
    QPainter p(&currPage);
    p.drawImage(QPoint(0,0), mask);
    p.end();
    emit statusSig("");
    update();
}

//
// Set deskew angle
//
void Viewer::setDeskew(double angle)
{
    Config::deskewAngle = angle;
    if (leftMode == Deskew)
        deskew();
}

//
// Calculate deskew angle
//
void Viewer::calcDeskew()
{
    if (currPage.isNull())
        return;

    PageData tmpImage = currPage;
    if (tmpImage.format() != QImage::Format_Grayscale8)
        tmpImage = tmpImage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat = QImage2OCV(tmpImage);

    // Convert to binary
    cv::Mat bin;
    cv::threshold(mat, bin, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // Convert to PIX
    PIX *pixS = pixCreate(bin.size().width, bin.size().height, 1);
    for(int i=0; i<bin.rows; i++)
        for(int j=0; j<bin.cols; j++)
            pixSetPixel(pixS, j, i, (l_uint32) bin.at<uchar>(i,j) & 1);

    // Find skew angle
    l_float32    angle, conf;
    if (pixFindSkew(pixS, &angle, &conf))
        Config::deskewAngle = 0.0;
    else
        Config::deskewAngle = angle;
    emit setDeskewWidget(Config::deskewAngle+0.05); // Make certain of a change output
    emit setDeskewWidget(Config::deskewAngle);
    pixDestroy(&pixS);
}

//
// Deskew image by small amount
//
void Viewer::deskew()
{
    QFuture<void> future = QtConcurrent::run(this, &Viewer::deskewThread);
    while (!future.isFinished())
    {
        QApplication::processEvents();
        QThread::msleep(1); //yield
    }
    future.waitForFinished();
}

//
// Run this in a separate thread to keep from blocking the UI
//
void Viewer::deskewThread()
{
    QMutexLocker locker(&mutex);

    if (currPage.isNull())
        return;

    // Blank these out
    fgMask = QImage();
    bgMask = QImage();
    emit statusSig("");

    // Rotate page
    QTransform tmat = QTransform().rotate(Config::deskewAngle);
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
    pushImage(currListItem, currPage);
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
    Config::despeckleArea = val;
    if (leftMode == Despeckle)
        despeckle();
}

//
// Slot to set devoid area
//
void Viewer::setDevoid(int val)
{
    Config::devoidArea = val;
    if (leftMode == Devoid)
        devoid();
}

//
// Calculate the despeckle mask
//
void Viewer::despeckle()
{
    blinkTimer->stop();

    QFuture<void> future = QtConcurrent::run(this, &Viewer::despeckleThread, currPage, Config::despeckleArea);
    while (!future.isFinished())
    {
        QApplication::processEvents();
        QThread::msleep(1); //yield
    }
    future.waitForFinished();

    // Blink mask
    blink = false;
    blinkTimer->start(500);
}

//
// Calculate the void mask
//
void Viewer::devoid()
{
    blinkTimer->stop();
    PageData invPage = currPage;
    invPage.invertPixels(QImage::InvertRgb);
    QFuture<void> future = QtConcurrent::run(this, &Viewer::despeckleThread, invPage, Config::devoidArea);
    while (!future.isFinished())
    {
        QApplication::processEvents();
        QThread::msleep(1); //yield
    }
    future.waitForFinished();

    // Blink mask
    blink = false;
    blinkTimer->start(500);
}

//
// Run this in a separate thread to keep from blocking the UI
//
void Viewer::despeckleThread(PageData page, int size)
{
    QMutexLocker locker(&mutex);

    // Nothing to do
    if (page.isNull())
        return;

    // Convert to grayscale
    QImage img;
    if ((page.format() == QImage::Format_RGB32) || (page.format() == QImage::Format_ARGB32) || (page.format() == QImage::Format_Mono))
        img = page.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
    else
        img = page;

    // Convert to OpenCV
    cv::Mat mat = QImage2OCV(img);

    // Make B&W with background black
    cv::Mat bw;
    cv::threshold(mat, bw, 250, 255, cv::THRESH_BINARY_INV);

    // Find blobs
    cv::Mat stats, centroids, labelImg;
    int nLabels = cv::connectedComponentsWithStats(bw, labelImg, stats, centroids, 4, CV_32S);

    // Build mask of blobs smaller than limit
    fgMask = QImage(labelImg.cols, labelImg.rows, QImage::Format_ARGB32);
    bgMask = QImage(labelImg.cols, labelImg.rows, QImage::Format_ARGB32);
    fgMask.fill(qRgba(0,0,0,0));
    bgMask.fill(qRgba(0,0,0,0));
    int cnt = 0;
    for(int idx=1; idx<nLabels; idx++)
    {
        // Check if this blob is small enough
        if (stats.at<int>(idx, cv::CC_STAT_AREA) <= size)
        {
            int top = stats.at<int>(idx, cv::CC_STAT_TOP);
            int bot = stats.at<int>(idx, cv::CC_STAT_TOP) + stats.at<int>(idx, cv::CC_STAT_HEIGHT);
            int left = stats.at<int>(idx, cv::CC_STAT_LEFT);
            int right = stats.at<int>(idx, cv::CC_STAT_LEFT) + stats.at<int>(idx, cv::CC_STAT_WIDTH);

            // Sweep the enclosing rectangle, setting pixels in the mask
            for (int row=top; row<bot; row++)
            {
                QRgb *fgMaskPtr = reinterpret_cast<QRgb*>(fgMask.scanLine(row));
                QRgb *bgMaskPtr = reinterpret_cast<QRgb*>(bgMask.scanLine(row));
                for (int col=left; col<right; col++)
                {
                    if (labelImg.at<int>(row,col) == idx)
                    {
                        fgMaskPtr[col] = foregroundColor.rgb();
                        bgMaskPtr[col] = backgroundColor.rgb();
                    }
                }
            }
            cnt++;
        }
    }
    emit statusSig(QStringLiteral("%1 blobs").arg(cnt));
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
// Convert current image to grayscale
//
void Viewer::toGrayscale()
{
    // Nothing to do
    if (currPage.isNull() || (currPage.format() == QImage::Format_Grayscale8))
        return;

    // Convert to grayscale
    pushImage(currListItem, currPage);
    PageData tmpImage = currPage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
    tmpImage.copyOtherData(currPage);
    currPage = tmpImage;

    // Record changes
    currPage.setChanges(currPage.changes() + 1);
    currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit imageChangedSig();
    update();
}

//
// Set the blur radius for binarization
//
void Viewer::setBlurRadius(int val)
{
    Config::blurRadius = val;

    // Even values crash openCV
    if (Config::blurRadius % 2 != 1)
        Config::blurRadius++;
    binarization(binMode);
}

//
// Set the kernel size for adaptive binarization
//
void Viewer::setKernelSize(int val)
{
    Config::kernelSize = val;

    // Even values crash openCV
    if (Config::kernelSize % 2 != 1)
        Config::kernelSize++;
    binarization(binMode);
}

void Viewer::toBinary()
{
    binMode = false;
    binarization(binMode);
}

void Viewer::toAdaptive()
{
    binMode = true;
    binarization(binMode);
}

//
// Convert to mono
//
void Viewer::binarization(bool adaptive)
{
    // Nothing to do
    if (currPage.isNull())
        return;

    // Run the conversion
    QFuture<void> future = QtConcurrent::run(this, 
            &Viewer::binThread, currListItem, Config::blurRadius, Config::kernelSize, adaptive);
    while (!future.isFinished())
    {
        QApplication::processEvents();
        QThread::msleep(1); //yield
    }
    future.waitForFinished();

    // Notify viewers
    currPage = currListItem->data(Qt::UserRole).value<PageData>();
    emit imageChangedSig();
    update();
}

//
// Run this in a separate thread to keep from blocking the UI
//
void Viewer::binThread(QListWidgetItem *listItem, int blur, int kernel, bool adaptive)
{
    // Don't start multiple threads
    QMutexLocker locker(&mutex);

    // If the last edit was binarization, rollback change and rerun
    PageData page = listItem->data(Qt::UserRole).value<PageData>();
    UndoBuffer ub = listItem->data(Qt::UserRole+1).value<UndoBuffer>();
    if ((page.format() == QImage::Format_Mono) && !ub.peek().isNull() && (ub.peek().format() != QImage::Format_Mono))
        page = ub.undo(page);
    ub.push(page);
    listItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));

    // Convert to grayscale
    PageData tmpImage = page;
    if (tmpImage.format() != QImage::Format_Grayscale8)
        tmpImage = tmpImage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat = QImage2OCV(tmpImage);

    // Gausian filter to clean up noise
    if (true)
    {
        cv::Mat tmp;
        cv::GaussianBlur(mat, tmp, cv::Size(blur, blur), 0);
        mat = tmp;
    }

    if (adaptive)   // Adaptive threshold - this hollows out diodes, etc
    {
        cv::Mat tmp;
        cv::adaptiveThreshold(mat, tmp, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, kernel, 2);
        mat = tmp;
    }
    else            // Otsu's global threshold calculation
    {
        cv::Mat tmp;
        cv::threshold(mat, tmp, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        mat = tmp;
    }

    // Convert back to QImage and reformat
    tmpImage = OCV2QImage(mat, page);
    PageData monoImage = tmpImage.convertToFormat(QImage::Format_Mono, Qt::MonoOnly|Qt::ThresholdDither|Qt::AvoidDither);
    monoImage.copyOtherData(page);

    // Record changes
    monoImage.setChanges(monoImage.changes() + 1);
    listItem->setData(Qt::UserRole, QVariant::fromValue(monoImage));
}

//
// Convert current image to dithered binary
//
void Viewer::toDithered()
{
    // Nothing to do
    if (currPage.isNull())
        return;

    // If the last edit was binarization, rollback change and rerun
    UndoBuffer ub = currListItem->data(Qt::UserRole+1).value<UndoBuffer>();
    if ((currPage.format() == QImage::Format_Mono) && !ub.peek().isNull() && (ub.peek().format() != QImage::Format_Mono))
        currPage = ub.undo(currPage);
    ub.push(currPage);
    currListItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));

    // Convert to grayscale
    PageData tmpImage = currPage.convertToFormat(QImage::Format_Mono, Qt::MonoOnly|Qt::DiffuseDither);
    tmpImage.copyOtherData(currPage);
    currPage = tmpImage;

    // Record changes
    currPage.setChanges(currPage.changes() + 1);
    currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit imageChangedSig();
    update();
}

//
// OCR selected region
//
void Viewer::regionOCR()
{
    if (currPage.isNull())
        return;
    if (rubberBand->isHidden())
    {
        QMessageBox::information(this, "OCR", "Area must be selected");
        return;
    }

    // Initialize the Tesseract API
    if (tessApi == nullptr)
    {
        tessApi = new tesseract::TessBaseAPI();
        if ((tessApi == nullptr) || (tessApi->Init(NULL, "eng")))
        {
            QMessageBox::information(this, "OCR", "Could not initialized OCR engine");
            return;
        }
    }

    // Initialize the clipboard
    if (clipboard == nullptr)
        clipboard = QGuiApplication::clipboard();

    //
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    // Get rectangle in image coordinates
    float scale = scaleBase * scaleFactor;
    QTransform transform = QTransform().scale(scale, scale).inverted();
    QRect box = transform.mapRect(rubberBand->geometry());

    // Copy region of interest
    QImage tmpImage = currPage.copy(box);

    // Convert to grayscale
    if (tmpImage.format() != QImage::Format_Grayscale8)
        tmpImage = tmpImage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat, bw;
    mat = QImage2OCV(tmpImage);
    cv::threshold(mat, bw, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    // OCR the selection
    //tessApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    tessApi->SetImage(bw.data, bw.cols, bw.rows, 1,bw.step);
    int res = 0.5 + currPage.dotsPerMeterX() / 39.3701;
    if ((res == 200) || (res == 300) || (res == 400) || (res == 600))
        tessApi->SetSourceResolution(res);
    clipboard->setText(tessApi->GetUTF8Text());
    QGuiApplication::restoreOverrideCursor();
}

//
// Warp image so warpCorner[] form a rectangle
//
void Viewer::doWarp()
{
    if (currPage.isNull())
        return;

    if (currPage.format() == QImage::Format_Mono)
    {
        QMessageBox::information(this, "Dewarp", "Only works on RGB and grayscale images");
        return;
    }

    // Sort points so 0 = top left, 1 = top right, 2 = bottom left, 3 = bottom right
    float cx = round((warpCorner[0].x + warpCorner[1].x + warpCorner[2].x + warpCorner[3].x) / 4.0);
    float cy = round((warpCorner[0].y + warpCorner[1].y + warpCorner[2].y + warpCorner[3].y) / 4.0);
    int flag = 0;
    for(int idx=0;idx<4;idx++)
    {
       if ((warpCorner[idx].x < cx) & (warpCorner[idx].y < cy))
       {
           warpBefore[0] = warpCorner[idx];
           flag |= 1;
       }
       else if ((warpCorner[idx].x > cx) & (warpCorner[idx].y < cy))
       {
           warpBefore[1] = warpCorner[idx];
           flag |= 2;
       }
       else if ((warpCorner[idx].x < cx) & (warpCorner[idx].y > cy))
       {
           warpBefore[2] = warpCorner[idx];
           flag |= 4;
       }
       else if ((warpCorner[idx].x > cx) & (warpCorner[idx].y > cy))
       {
           warpBefore[3] = warpCorner[idx];
           flag |= 8;
       }
    }
    if (flag != 15)
    {
        QMessageBox::information(this, "Dewarp", "Points too close together. Couldn't find centroid");
        return;
    }

    // Convert to OCV
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    pushImage(currListItem, currPage);
    cv::Mat mat = QImage2OCV(currPage);

    // Compute perspective transform
    float left = round((warpBefore[0].x + warpBefore[2].x) / 2.0);
    float right = round((warpBefore[1].x + warpBefore[3].x) / 2.0);
    float top = round((warpBefore[0].y + warpBefore[1].y) / 2.0);
    float bottom = round((warpBefore[2].y + warpBefore[3].y) / 2.0);
    warpAfter[0] = cv::Point2f(left, top);
    warpAfter[1] = cv::Point2f(right, top);
    warpAfter[2] = cv::Point2f(left, bottom);
    warpAfter[3] = cv::Point2f(right, bottom);
    cv::Mat transform = getPerspectiveTransform( warpBefore, warpAfter );

    // Warp image
    cv::Size rect = cv::Size(currPage.width(), currPage.height());
    cv::Mat warpedImage;
    cv::warpPerspective( mat, warpedImage, transform, rect, cv::INTER_LANCZOS4, cv::BORDER_REPLICATE );

    // Convert back
    currPage = OCV2QImage(warpedImage, currPage);

    // Record changes
    currPage.setChanges(currPage.changes() + 1);
    currListItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit imageChangedSig();
    update();
    QGuiApplication::restoreOverrideCursor();
}

//
// Save current image to undo buffer
//
void Viewer::pushImage(QListWidgetItem *listItem, PageData &page)
{
    if (page.isNull())
        return;

    UndoBuffer ub = listItem->data(Qt::UserRole+1).value<UndoBuffer>();
    ub.push(page);
    listItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));
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
