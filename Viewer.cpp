#include "Config.h"
#include "Viewer.h"
#include "Utils/QImage2OCV.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>

Viewer::Viewer(QWidget * parent) : QWidget(parent)
{
    setFocusPolicy(Qt::WheelFocus);
    setScaleFactor(1.0);

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

// Capture/Release keyboard
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
    if (currPage.m_img.isNull())
        return;

    // Grab keyboard modifiers
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);

    // Event handled flag
    bool flag = false;

    // If left mouse button pressed
    if (event->button() == Qt::LeftButton)
    {
        leftOrigin = event->pos();
        if (pasting)
        {
            flag = true;
        }
        else if (leftMode == Select)
        {
            LMRBstart = scrnToPage.map(leftOrigin);
            LMRBend = LMRBstart;
            leftBand->setGeometry(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)));
            leftBand->show();
            flag = true;
        }
        else if ((leftMode == Pencil) || (leftMode == Eraser))
        {
            currColor = (leftMode == Pencil) ? Config::fgColor : Config::bgColor;
            currPage.push();
            if (!shift)
                drawDot(leftOrigin, currColor);
            flag = true;
        }
    }

    // If right mouse button pressed
    if (event->button() == Qt::RightButton)
    {
        rightOrigin = event->pos();
        if (shift)
        {
            lastCursor = cursor();
            setCursor(Qt::OpenHandCursor);
            rightMode = Pan;
        }
        else
        {
            RMRBstart = scrnToPage.map(rightOrigin);
            RMRBend = RMRBstart;
            rightBand->setGeometry(QRect(pageToScrn.map(RMRBstart), pageToScrn.map(RMRBend)));
            rightBand->show();
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
    if (currPage.m_img.isNull())
        return;

    // Grab keyboard modifiers
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);
    bool ctrl = keyMod.testFlag(Qt::ControlModifier);

    // Event handled flag
    bool flag = false;

    // If pasting, update location
    if (pasting)
    {
        pasteLoc = pasteLocator(event->pos(), ctrl);
        update();
        flag = true;
    }

    // If left mouse button is pressed
    if (event->buttons() & Qt::LeftButton)
    {
        if (leftMode == Select)
        {
            LMRBend = scrnToPage.map(event->pos());
            leftBand->setGeometry(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)).normalized());
            flag = true;
        }
        else if ((leftMode == Pencil) || (leftMode == Eraser))
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
                drawLine(leftOrigin, event->pos(), currColor);
                leftOrigin = event->pos();
            }
            flag = true;
        }
    }

    // If right mouse button pressed
    if (event->buttons() & Qt::RightButton)
    {
        if (rightMode == Pan)
        {
            QPoint delta = event->pos() - rightOrigin;
            scrollArea->horizontalScrollBar()->setValue(
                scrollArea->horizontalScrollBar()->value() - delta.x());
            scrollArea->verticalScrollBar()->setValue(
                scrollArea->verticalScrollBar()->value() - delta.y());
        }
        else
        {
            RMRBend = scrnToPage.map(event->pos());
            rightBand->setGeometry(QRect(pageToScrn.map(RMRBstart), pageToScrn.map(RMRBend)).normalized());
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
    if (currPage.m_img.isNull())
        return;

    // Grab keyboard modifiers
    Qt::KeyboardModifiers keyMod = event->modifiers();
    bool shift = keyMod.testFlag(Qt::ShiftModifier);

    // Event handled flag
    bool flag = false;

    // If left mouse button was released
    if (event->button() == Qt::LeftButton)
    {
        if (pasting)
        {
            doPaste(shift);
            flag = true;
        }
        else if (leftMode == Select)
        {
            flag = true;
        }
        else if ((leftMode == Pencil) || (leftMode == Eraser))
        {
            shiftPencil = false;
            drawLine(leftOrigin, event->pos(), currColor);
            currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit updateIconSig();
            flag = true;
        }
        else if (leftMode == ColorSelect)
        {
            leftOrigin = event->pos();
            doDropper();
            flag = true;
        }
        else if (leftMode == FloodFill)
        {
            leftOrigin = event->pos();
            doFlood();
            flag = true;
        }
        else if (leftMode == RemoveBG)
        {
            doRemoveBG();
            flag = true;
        }
        else if (leftMode == Despeckle)
        {
            doDespeckle();
            flag = true;
        }
        else if (leftMode == Devoid)
        {
            doDevoid();
            flag = true;
        }
        else if (leftMode == LocateRef)
        {
            // Legalize point to inside image
            QPoint loc = scrnToPageOffs.map(leftOrigin);
            loc.setX(std::min( std::max(loc.x(), 0), currPage.m_img.width()-1) );
            loc.setY(std::min( std::max(loc.y(), 0), currPage.m_img.height()-1) );

            // Save location
            if (locateShift)
                Config::locate2 = loc;
            else
                Config::locate1 = loc;

            // Reset tool to exit mode
            setTool(Select);
            flag = true;
        }
        else if (leftMode == PlaceRef)
        {
            // Legalize point to inside image
            QPoint loc = scrnToPageOffs.map(leftOrigin);
            loc.setX(std::min( std::max(loc.x(), 0), currPage.m_img.width()-1) );
            loc.setY(std::min( std::max(loc.y(), 0), currPage.m_img.height()-1) );

            // Compute delta
            QPointF delta;
            if (locateShift)
                delta = Config::locate2 - loc;
            else
                delta = Config::locate1 - loc;

            // Paint shift image over original
            currPage.push();
            QImage img = currPage.m_img;
            QPainter p(&currPage.m_img);
            p.fillRect(currPage.m_img.rect(), Config::bgColor);
            p.drawImage(delta, img);
            p.end();

            // Update icon
            currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit updateIconSig();
            update();
            flag = true;
        }
    }

    // If right mouse button was released
    if (event->button() == Qt::RightButton)
    {
        if (rightMode == Pan)
        {
            setCursor(lastCursor);
        }
        else
        {
            rightBand->hide();
            zoomArea(QRect(pageToScrn.map(RMRBstart), pageToScrn.map(RMRBend)).normalized());
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
    if (currPage.m_img.isNull())
        return;

    if (event->angleDelta().y() > 0)
        zoomWheel(event->pos(), 1.25);
    else
        zoomWheel(event->pos(), 0.8);
    event->accept();
}

//
// Compare against standard key sequences
//     Based on QKeyEvent::matches(...)
//
Viewer::MatchCode Viewer::keyMatches(QKeyEvent *event, QKeySequence::StandardKey matchKey)
{
    uint key = event->key();
    uint mods = event->modifiers();

    // Exact match
    uint searchKey = (mods | key) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    QList<QKeySequence> bindings = QKeySequence::keyBindings(matchKey);
    if (bindings.contains(QKeySequence(searchKey)))
        return Exact;

    // Match plus Shift
    searchKey = ((mods & ~Qt::ShiftModifier) | key) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    bindings = QKeySequence::keyBindings(matchKey);
    if (bindings.contains(QKeySequence(searchKey)))
        return Shifted;

    // Match plus Control
    searchKey = ((mods & ~Qt::ControlModifier) | key) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    bindings = QKeySequence::keyBindings(matchKey);
    if (bindings.contains(QKeySequence(searchKey)))
        return Ctrled;

    return None;
}

//
// Handle key presses
//
void Viewer::keyPressEvent(QKeyEvent *event)
{
    if (currPage.m_img.isNull())
        return;

    uint key = event->key();
    bool shft = event->modifiers().testFlag(Qt::ShiftModifier);
    bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    bool flag = false;

    if (key == Qt::Key_Escape)
    {
        resetTools();
        flag = true;
    }
    else if (key == Qt::Key_F)
    {
        emit zoomSig();
        flag = true;
    }
    else if (event->matches(QKeySequence::Undo))
    {
        bool updateZoom = currPage.undo();
        currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
        emit updateIconSig();
        if (updateZoom)
            fitWindow();
        update();
        flag = true;
    }
    else if (event->matches(QKeySequence::Redo))
    {
        bool updateZoom = currPage.redo();
        currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
        emit updateIconSig();
        if (updateZoom)
            fitWindow();
        update();
        flag = true;
    }
    else if (keyMatches(event, QKeySequence::Cut) != None)
    {
        if (!pageMask.isNull())
        {
            blinkTimer->stop();
            currPage.push();
            currPage.applyMask(pageMask, Config::bgColor);
            currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit updateIconSig();
            resetTools();
            update();
        }
        else if (leftBand->isHidden())
        {
            QMessageBox::information(this, "Cut", "Area must be selected");
        }
        else
        {
            leftBand->hide();
            if (shft)
                fillArea(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)).normalized(), Config::fgColor, false);
            else
                fillArea(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)).normalized(), Config::bgColor, false);
        }
        flag = true;
    }
    else if (ctrl & (key == Qt::Key_S))
    {
        if (!pageMask.isNull())
        {
            blinkTimer->stop();
            currPage.push();
            currPage.applyMask(pageMask, Config::fgColor);
            currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit updateIconSig();
            resetTools();
            update();
        }
        else if (leftBand->isHidden())
        {
            QMessageBox::information(this, "Cut", "Area must be selected");
        }
        else
        {
            leftBand->hide();
            if (shft)
                fillArea(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)).normalized(), Config::fgColor, true);
            else
                fillArea(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)).normalized(), Config::bgColor, true);
        }
        flag = true;
    }
    else if (keyMatches(event, QKeySequence::Copy) != None)
    {
        if (leftBand->isHidden())
        {
            QMessageBox::information(this, "Copy", "Area must be selected");
        }
        else
        {
            leftBand->hide();
            doCopy(QRect(LMRBstart, LMRBend).normalized());
        }
        flag = true;
    }
    else if (keyMatches(event, QKeySequence::Paste) != None)
    {
        if (!deskewImg.isNull())
        {
            currPage.push();
            currPage.applyDeskew(deskewImg);
            currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit updateIconSig();
            leftMode = Select;
            resetTools();
            update();
        }
        else
        {
            // Don't optimize location since Ctrl may already be down
            pasteLoc = pasteLocator(mapFromGlobal(cursor().pos()), false);
            setupPaste();
        }
        flag = true;
    }
    else if (key == Qt::Key_T)
    {
        if (ctrl)
        {
            if (leftBand->isHidden())
            {
                QMessageBox::information(this, "OCR", "Area must be selected");
            }
            else
            {
                leftBand->hide();
                doRegionOCR(QRect(LMRBstart, LMRBend).normalized());
            }
        }
        else if ((leftMode == Pencil) || (leftMode == Eraser))
        {
            pencil180 = !pencil180;
            setCursor(pencil180 ? Pencil180Cursor : PencilCursor);
        }
        flag = true;
    }
    else if (ctrl && (key == Qt::Key_R))
    {
        locateShift = shft;
        setTool(LocateRef);
        flag = true;
    }
    else if (ctrl && (key == Qt::Key_E))
    {
        locateShift = shft;
        setTool(PlaceRef);
        flag = true;
    }
    else if (pasting)
    {
        if (keyMatches(event, QKeySequence::Delete) == Exact)
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
                resetTools();
            }
        }
        else
            pasteLoc = pasteLocator(mapFromGlobal(cursor().pos()), ctrl);
        update();
        flag = true;
    }
    else if (!deskewImg.isNull())
    {
        if (key == Qt::Key_Down)
        {
            gridOffsetY++;
            while (gridOffsetY > 50)
                gridOffsetY -= 50;
            update();
            flag = true;
        }
        else if (key == Qt::Key_Up)
        {
            gridOffsetY--;
            while (gridOffsetY < 0)
                gridOffsetY += 50;
            update();
            flag = true;
        }
        else if (key == Qt::Key_Right)
        {
            gridOffsetX++;
            while (gridOffsetX > 50)
                gridOffsetX -= 50;
            update();
            flag = true;
        }
        else if (key == Qt::Key_Left)
        {
            gridOffsetX--;
            while (gridOffsetX < 0)
                gridOffsetX += 50;
            update();
            flag = true;
        }
    }

    // Event was handled
    if (flag)
        event->accept();
    else
        QWidget::keyPressEvent(event);
}

//
// Handle key releases
//
void Viewer::keyReleaseEvent(QKeyEvent *event)
{
    if (currPage.m_img.isNull())
        return;

    bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    bool flag = false;

    if (pasting)
    {
        pasteLoc = pasteLocator(mapFromGlobal(cursor().pos()), ctrl);
        update();
        flag = true;
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

    if (currPage.m_img.isNull())
        p.drawImage((rect().bottomRight() - logo.rect().bottomRight())/2.0, logo);
    else
    {
        // Adjust rubberbands for scale
        if (!leftBand->isHidden())
            leftBand->setGeometry(QRect(pageToScrn.map(LMRBstart), pageToScrn.map(LMRBend)).normalized());
        if (!rightBand->isHidden())
            rightBand->setGeometry(QRect(pageToScrn.map(RMRBstart), pageToScrn.map(RMRBend)).normalized());

        // Draw page
        p.setTransform(pageToScrn);
        p.drawImage(currPage.m_img.rect().topLeft(), currPage.m_img);

        // Additions to page
        if (pasting)
        {
            p.setOpacity(0.3);
            p.drawImage(pasteLoc, copyImage);
        }
        else if (!pageMask.isNull())
        {
            p.drawImage(QPoint(0,0), pageMask);
        }
        else if (!deskewImg.isNull())
        {
            // Draw deskewed image rotated about the center
            QRect rect(deskewImg.rect());
            rect.moveCenter(currPage.m_img.rect().center());
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
            QPointF start = scrnToPage.map(leftOrigin);
            QPointF finish = scrnToPage.map(drawLoc);
            p.setPen(QPen(currColor, Config::brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(start, finish);
        }
    }
    p.end();
}

//
// Change Page
//
void Viewer::changePage(QListWidgetItem *curr)
{
    // Save current view
    if (currItem != nullptr)
    {
        currPage = currItem->data(Qt::UserRole).value<Page>();
        currPage.scaleFactor = scaleFactor;
        currPage.horizontalScroll = scrollArea->horizontalScrollBar()->value();
        currPage.verticalScroll = scrollArea->verticalScrollBar()->value();
        currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    }

    // Load new page
    if (curr != nullptr)
    {
        currItem = curr;
        currPage = currItem->data(Qt::UserRole).value<Page>();

        // Restore view position
        if (currPage.scaleFactor != 0.0)
        {
            setScaleFactor(currPage.scaleFactor);
            updateGeometry();
            updateScrollBars();
            scrollArea->horizontalScrollBar()->setValue(currPage.horizontalScroll);
            scrollArea->verticalScrollBar()->setValue(currPage.verticalScroll);
        }
        else
        {
            fitWindow();
        }
    }
    else
    {
        currItem = nullptr;
        currPage = Page();
    }

    // Cleanup
    leftBand->hide();
    rightBand->hide();
    update();
}

//
// Page has changed, redraw
//
void Viewer::updatePage(bool updateZoom)
{
    if (currItem == nullptr)
        return;
    currPage = currItem->data(Qt::UserRole).value<Page>();
    if (updateZoom)
        fitWindow();
    update();
}

//
// Change tool for left mouse button
//
void Viewer::setTool(LeftMode tool)
{
    if (Config::multiPage)
    {
        leftMode = Select;
        resetTools();
        setCursor(Qt::ArrowCursor);
        update();
        return;
    }

    leftMode = tool;
    resetTools();
    if ((tool == Pencil) || (tool == Eraser))
    {
        pencil180 = false;
        setCursor(pencil180 ? Pencil180Cursor : PencilCursor);
    }
    else if ((tool == ColorSelect) || (tool == FloodFill))
        setCursor(DropperCursor);
    else if (tool == Despeckle)
    {
        setCursor(DespeckleCursor);
        doDespeckle();
    }
    else if (tool == Devoid)
    {
        setCursor(DespeckleCursor);
        doDevoid();
    }
    else if (tool == RemoveBG)
        doRemoveBG();
    else if (tool == Deskew)
    {
        Config::deskewAngle = currPage.calcDeskew();
        emit setDeskewSig(Config::deskewAngle + 0.05);
        emit setDeskewSig(Config::deskewAngle);
    }
    else if ((tool == PlaceRef) || (tool == LocateRef))
        setCursor(Qt::CrossCursor);
    else
        setCursor(Qt::ArrowCursor);
    update();
}

//
// Reset tools
//
void Viewer::resetTools()
{
    pasting = false;
    deskewImg = QImage();
    pageMask = QImage();
}

//
// Draw a line in the foreground color
//
void Viewer::drawLine(QPoint start, QPoint finish, QColor color)
{
    if (currPage.m_img.isNull())
        return;

    QPainter p(&currPage.m_img);
    p.setTransform(scrnToPage);
    p.setPen(QPen(color, int(Config::brushSize * scaleFactor + 0.5), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(start, finish);
    p.end();
    update();
}

//
// Draw a dot in the foreground color
//
void Viewer::drawDot(QPoint loc, QColor color)
{
    if (currPage.m_img.isNull())
        return;

    // Ellipse doesn't work well for single pixels
    if (Config::brushSize == 1)
    {
        currPage.m_img.setPixelColor(scrnToPageOffs.map(loc), color);
    }
    else
    {
        QPainter p(&currPage.m_img);
        p.setTransform(scrnToPage);
        p.setBrush(color);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(QPen(color, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawEllipse(loc, int(Config::brushSize * scaleFactor/2.0 + 0.5), int(Config::brushSize * scaleFactor/2.0 + 0.5));
        p.end();
    }
    update();
}

//
// Fill area with color
//
void Viewer::fillArea(QRect rect, QColor color, bool outside)
{
    if (currPage.m_img.isNull())
        return;

    currPage.push();
    QPainter p(&currPage.m_img);
    if (outside)
    {
        QRect box = scrnToPage.mapRect(rect);
        int imgW = currPage.m_img.size().width();
        int imgH = currPage.m_img.size().height();

        p.fillRect(0, 0, imgW, box.top(), color);
        p.fillRect(0, 0, box.left(), imgH, color);
        p.fillRect(box.right(), 0, imgW, imgH, color);
        p.fillRect(0, box.bottom(), imgW, imgH, color);
    }
    else
    {
        p.setTransform(scrnToPage);
        p.fillRect(rect, color);
    }
    p.end();
    currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit updateIconSig();
    update();
}

//
// Copy the region
//
void Viewer::doCopy(QRect box)
{
    copyImage = currPage.m_img.copy(box);
    copyImageList.prepend(copyImage);
    if (copyImageList.size() > 12)
        copyImageList.removeLast();
}

//
// Setup paste of copyied region
//
void Viewer::setupPaste()
{
    if (copyImage.isNull())
        return;

    // If already pasting, switch to next image in list
    if (pasting)
    {
        int idx = copyImageList.indexOf(copyImage);
        if ((idx + 1) < copyImageList.size())
            copyImage = copyImageList.at(idx+1);
        else
            copyImage = copyImageList.at(0);
    }

    // Turn on pasting
    pasting = true;
    setMouseTracking(true);
    update();
}

//
// Paste copyImage into page
//
void Viewer::doPaste(bool transparent)
{
    // Cleanup
    pasting = false;
    setMouseTracking(false);
    currPage.push();

    // Bump copyImage to head of list
    int idx = copyImageList.indexOf(copyImage);
    if (idx > 0)
        copyImageList.move(idx, 0);

    // Paint the copied section
    QPainter p(&currPage.m_img);
    if (transparent)
    {
        // If shift is pressed, convert everything close to white into transparent
        QImage tmp = copyImage.copy();
        if (copyImage.format() != QImage::Format_ARGB32)
            tmp = copyImage.convertToFormat(QImage::Format_ARGB32);
        QRgb transparent = qRgba(0,0,0,0);
        for(int i=0; i<tmp.height(); i++)
        {
            QRgb *srcPtr = (QRgb *)tmp.scanLine(i);
            for(int j=0; j<tmp.width(); j++)
            {
                QRgb val = *srcPtr;
                if ((qRed(val) >= 240) && (qGreen(val) >= 240) && (qBlue(val) >= 240))
                    *srcPtr = transparent;
                srcPtr++;
            }
        }
        p.drawImage(pasteLoc, tmp);
    }
    else
        p.drawImage(pasteLoc, copyImage);
    p.end();
    currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit updateIconSig();
    update();
}

//
// Compute the location of the paste image from cursor position
//
QPoint Viewer::pasteLocator(QPoint mouse, bool optimize)
{
    // Map to page coordinates
    QPointF loc = scrnToPage.map(mouse);

    // Offset from center to upper left corner
    qreal imgW = copyImage.size().width();
    qreal imgH = copyImage.size().height();
    loc = loc - QPointF(imgW/2.0, imgH/2.0);

    if (optimize)
    {
        // Find position that has highest corelation
        int win = 4;

        // Convert area around mouse to grayscale
        QImage tmp1 = currPage.m_img.copy(loc.x() - win, loc.y() - win, imgW + win*2, imgH + win*2);
        if (tmp1.format() != QImage::Format_Grayscale8)
            tmp1 = tmp1.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
        cv::Mat mat1 = QImage2OCV(tmp1);

        // Convert paste image to grayscale
        QImage tmp2 = copyImage;
        if (tmp2.format() != QImage::Format_Grayscale8)
            tmp2 = tmp2.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
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
        return QPoint( loc.x() + matchLoc.x - win, loc.y() + matchLoc.y - win );
    }
    return QPoint( loc.x(), loc.y() );
}

//
// Execute color select in response to mouse click or threshold change
//
void Viewer::doDropper()
{
    if (leftMode != ColorSelect)
        return;

    // Legalize point to inside image
    QPoint loc = scrnToPageOffs.map(leftOrigin);
    loc.setX(std::min( std::max(loc.x(), 0), currPage.m_img.width()-1) );
    loc.setY(std::min( std::max(loc.y(), 0), currPage.m_img.height()-1) );

    // Get pixel under cursor
    QRgb pixel = currPage.m_img.pixel(loc);
    blinkTimer->stop();
    resetTools();
    pageMask = currPage.colorSelect(pixel, Config::dropperThreshold);
    blinkTimer->start(300);
    update();
}

//
// Execute flood fill in response to mouse click or threshold change
//
void Viewer::doFlood()
{
    if (leftMode != FloodFill)
        return;

    // Legalize point to inside image
    QPoint loc = scrnToPageOffs.map(leftOrigin);
    loc.setX(std::min( std::max(loc.x(), 0), currPage.m_img.width()-1) );
    loc.setY(std::min( std::max(loc.y(), 0), currPage.m_img.height()-1) );

    blinkTimer->stop();
    resetTools();
    pageMask = currPage.floodFill(loc, Config::floodThreshold);
    blinkTimer->start(300);
    update();
}

//
// Execute background removal in response to mouse click or threshold change
//
void Viewer::doRemoveBG()
{
    if (leftMode != RemoveBG)
        return;

    blinkTimer->stop();
    resetTools();
    pageMask = currPage.colorSelect(QColor(Qt::white).rgb(), Config::bgRemoveThreshold);
    blinkTimer->start(300);
    update();
}

//
// Execute speckle removal in response to mouse click or area change
//
void Viewer::doDespeckle()
{
    if (leftMode != Despeckle)
        return;

    blinkTimer->stop();
    resetTools();
    pageMask = currPage.despeckle(Config::despeckleArea, false);
    blinkTimer->start(300);
    update();
}

//
// Execute void removal in response to mouse click or area change
//
void Viewer::doDevoid()
{
    if (leftMode != Devoid)
        return;

    blinkTimer->stop();
    resetTools();
    pageMask = currPage.despeckle(Config::devoidArea, true);
    blinkTimer->start(300);
    update();
}

//
// Update image based on deskew angle
//
void Viewer::doDeskew()
{
    if (leftMode != Deskew)
        return;

    resetTools();
    deskewImg = currPage.deskew(Config::deskewAngle);
    update();
}

//
// Convert to grayscale
//
void Viewer::toGrayscale()
{
    if (currPage.m_img.isNull())
        return;
    if (Config::multiPage)
        return;

    currPage.push();
    currPage.toGrayscale();
    currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit updateIconSig();
    update();
}

//
// Convert to binary using Otsu's algorithm
//
void Viewer::toBinary()
{
    if (currPage.m_img.isNull())
        return;
    if (Config::multiPage)
        return;

    // If last operation converted to mono, undo it
    if ((currPage.m_img.format() == QImage::Format_Mono) && (currPage.peek().format() != QImage::Format_Mono))
        currPage.undo();

    currPage.push();
    currPage.toBinary(false);
    currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit updateIconSig();
    update();
}

//
// Convert to binary using adaptive threshold
//
void Viewer::toAdaptive()
{
    if (currPage.m_img.isNull())
        return;
    if (Config::multiPage)
        return;

    // If last operation converted to mono, undo it
    if ((currPage.m_img.format() == QImage::Format_Mono) && (currPage.peek().format() != QImage::Format_Mono))
        currPage.undo();

    currPage.push();
    currPage.toBinary(true);
    currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit updateIconSig();
    update();
}

//
// Convert to binary using diffuse dithering
//
void Viewer::toDithered()
{
    if (currPage.m_img.isNull())
        return;
    if (Config::multiPage)
        return;

    // If last operation converted to mono, undo it
    if ((currPage.m_img.format() == QImage::Format_Mono) && (currPage.peek().format() != QImage::Format_Mono))
        currPage.undo();

    currPage.push();
    currPage.toDithered();
    currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
    emit updateIconSig();
    update();
}

//
// OCR selection rectangle
//
void Viewer::doRegionOCR(QRect rect)
{
    if (currPage.m_img.isNull())
        return;

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

    // Set cursor
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    // Copy region of interest
    QImage img = currPage.m_img.copy(rect);

    // Convert to grayscale
    if (img.format() != QImage::Format_Grayscale8)
        img = img.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat, bw;
    mat = QImage2OCV(img);
    cv::threshold(mat, bw, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    // OCR the selection
    //tessApi->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    tessApi->SetImage(bw.data, bw.cols, bw.rows, 1, bw.step);
    int res = 0.5 + img.dotsPerMeterX() / 39.3701;
    if ((res == 200) || (res == 300) || (res == 400) || (res == 600))
        tessApi->SetSourceResolution(res);
    clipboard->setText(tessApi->GetUTF8Text());

    // Restore cursor
    QGuiApplication::restoreOverrideCursor();
}

//
// Blink mask to make it easier to see
//
void Viewer::blinker()
{
    if (currPage.m_img.isNull() || pageMask.isNull())
    {
        blinkTimer->stop();
        return;
    }
    if (pageMask.color(0) != Config::fgColor.rgba())
        pageMask.setColor( 0, Config::fgColor.rgba() );
    else
        pageMask.setColor( 0, Config::bgColor.rgba() );
    update();
}

//
// Always honor aspect ratio when resizing
//
QSize Viewer::sizeHint() const
{
    if (currPage.m_img.isNull())
        return logo.size();
    return currPage.m_img.size() * scaleFactor;
}

//
// Set scale factor and transforms
//
void Viewer::setScaleFactor(float val)
{
    // Too much zoom causes crashes
    // Should be calculated from image size vs window size
    if (val > 10000)
        val = 10000;
    scaleFactor = val;
    pageToScrn = QTransform::fromScale(scaleFactor, scaleFactor);
    scrnToPage = pageToScrn.inverted();
    scrnToPageOffs = pageToScrn.inverted().translate(-scaleFactor*0.5,-scaleFactor*0.5);
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
    if (currPage.m_img.isNull())
        return;
    setScaleFactor(scaleFactor * 1.25);
    updateGeometry();
    updateScrollBars();
    adjustScrollBars(1.25);
}

//
// Draw image 20% smaller
void Viewer::zoomOut()
{
    if (currPage.m_img.isNull())
        return;
    setScaleFactor(scaleFactor * 0.8);
    updateGeometry();
    updateScrollBars();
    adjustScrollBars(0.8);
}

//
// Zoom to rubberband rectangle
//
void Viewer::zoomArea(QRect rect)
{
    if (currPage.m_img.isNull())
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
        setScaleFactor(scaleFactor * (float)viewW / rectW);
    else
        setScaleFactor(scaleFactor * (float)viewH / rectH);
    updateGeometry();
    updateScrollBars();

    // Adjust scrollbars so center of rect is center of viewport
    scrollArea->horizontalScrollBar()->setValue(int(
                centerX * scrollArea->horizontalScrollBar()->maximum() +
                ((centerX - 0.5) * scrollArea->horizontalScrollBar()->pageStep())));
    scrollArea->verticalScrollBar()->setValue(int(
                centerY * scrollArea->verticalScrollBar()->maximum() +
                ((centerY - 0.5) * scrollArea->verticalScrollBar()->pageStep())));
}

//
// Zoom to wheel event
//
void Viewer::zoomWheel(QPoint pos, float factor)
{
    if (currPage.m_img.isNull())
        return;

    // Save for later
    int hVal = scrollArea->horizontalScrollBar()->value();
    int vVal = scrollArea->verticalScrollBar()->value();

    // Apply the zoom
    setScaleFactor(scaleFactor * factor);
    updateGeometry();
    updateScrollBars();
    scrollArea->horizontalScrollBar()->setValue(int(hVal + (factor - 1) * pos.x()));
    scrollArea->verticalScrollBar()->setValue(int(vVal + (factor - 1) * pos.y()));
}

//
// Helper function for window fit functions
//     returns true if horizontal dimension is larger
//
bool Viewer::measureAll(Page &page, int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH)
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
    imageW = page.m_img.size().width();
    imageH = page.m_img.size().height();

    // Scale to horizontal if it is larger
    if (viewW * imageH > viewH * imageW)
        return true;
    return false;
}

//
// Fit image to window without scrollbars
//
void Viewer::fitWindow()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.m_img.isNull())
        return;

    // Scale to larger dimension
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        setScaleFactor((float)viewH / imageH);
    else
        setScaleFactor((float)viewW / imageW);

    // Update scrollarea
    updateGeometry();
}

//
// Fit image to window with only a vertical scrollbar
//
void Viewer::fitWidth()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.m_img.isNull())
        return;

    // If height is larger dimension, leave space for vertical scroll bar
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        setScaleFactor((float)(viewW - scrollBarSize) / imageW);
    else
        setScaleFactor((float)viewW / imageW);

    // Update scrollarea
    updateGeometry();
}

//
// Fit image to window with only a horizontal scrollbar
//
void Viewer::fitHeight()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.m_img.isNull())
        return;

    // If width is larger dimension, leave space for horizontal scroll bar
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        setScaleFactor((float)(viewH - scrollBarSize) / imageH);
    else
        setScaleFactor((float)viewH / imageH);

    // Update scrollarea
    updateGeometry();
}

//
// Fit image to window with a maximum of one scrollbar
//
void Viewer::fillWindow()
{
    int scrollBarSize, viewW, viewH, imageW, imageH;

    if (currPage.m_img.isNull())
        return;

    // Scale to smaller dimension
    if (measureAll(currPage, scrollBarSize, viewW, viewH, imageW, imageH))
        fitWidth();
    else
        fitHeight();
}
