#include "Config.h"
#include "Viewer.h"
#include "Utils/QImage2OCV.h"
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
}

Viewer::~Viewer()
{
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
            setCursor(pencil180 ? Pencil180Cursor : PencilCursor);
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
            setCursor(Qt::ArrowCursor);
            currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
            emit updateIconSig();
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
        pasting = false;
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
            fitToWindow();
        update();
        flag = true;
    }
    else if (event->matches(QKeySequence::Redo))
    {
        bool updateZoom = currPage.redo();
        currItem->setData(Qt::UserRole, QVariant::fromValue(currPage));
        emit updateIconSig();
        if (updateZoom)
            fitToWindow();
        update();
        flag = true;
    }
    else if (keyMatches(event, QKeySequence::Cut) != None)
    {
        if (leftBand->isHidden())
        {
            QMessageBox::information(this, "Cut", "Area must be selected");
            return;
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
        if (leftBand->isHidden())
        {
            QMessageBox::information(this, "Cut", "Area must be selected");
            return;
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
            return;
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
        // Don't optimize location since Ctrl may already be down
        pasteLoc = pasteLocator(mapFromGlobal(cursor().pos()), false);
        setupPaste();
        flag = true;
    }
    else if (pasting)
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
            fitToWindow();
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
    currPage = currItem->data(Qt::UserRole).value<Page>();
    if (updateZoom)
        fitToWindow();
    update();
}

//
// Change tool for left mouse button
//
void Viewer::setTool(LeftMode tool)
{
    setCursor(Qt::ArrowCursor);
    leftMode = tool;
    if ((tool == Pencil) || (tool == Eraser))
        pencil180 = false;
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

    QPainter p(&currPage.m_img);
    p.setTransform(scrnToPage);
    p.setBrush(color);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(color, 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawEllipse(loc, int(Config::brushSize * scaleFactor/2.0 + 0.5), int(Config::brushSize * scaleFactor/2.0 + 0.5));
    p.end();
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
void Viewer::fitToWindow()
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
