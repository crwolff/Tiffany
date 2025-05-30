// Page.cpp

#include "Page.h"
#include "Utils/QImage2OCV.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>
#include <leptonica/allheaders.h>
#include <math.h>

// Constuctors
Page::Page()
{
    m_img = QImage();
}

Page::Page(const QString &fileName, const char *format)
{
    m_img = QImage(fileName, format);
}

Page::Page(const QImage &image)
{
    m_img = image;
}

Page::~Page()
{
}

//
// Indicates if page was changed since loading or last save
//
bool Page::modified()
{
    if (m_modified > 0)
        return true;
    return false;
}

//
// Flush undo buffer
//
void Page::flush()
{
    m_undo.clear();
    m_redo.clear();
    m_modified = 0;
}

//
// Save current image to undo buffer
//
void Page::push()
{
    m_undo.insert(0, m_img);
    if (m_undo.count() >  MAX_UNDO)
        m_undo.removeLast();
    m_redo.clear();
    m_modified++;
}

//
// Undo last edit
//
bool Page::undo()
{
    QSize oldSize = m_img.size();
    if (m_undo.count() > 0)
    {
        m_redo.insert(0, m_img);
        m_img = m_undo.takeFirst();
        m_modified--;
    }
    return (m_img.size() != oldSize);
}

//
// Redo last undo
//
bool Page::redo()
{
    QSize oldSize = m_img.size();
    if (m_redo.count() > 0)
    {
        m_undo.insert(0, m_img);
        m_img = m_redo.takeFirst();
        m_modified++;
    }
    return (m_img.size() != oldSize);
}

//
// Peek at last image
//
QImage Page::peek()
{
    if (m_undo.count() > 0)
        return m_undo.first();
    return QImage();
}

//
// Select all pixels near the cursor's color
//
QImage Page::colorSelect(QRgb target, int threshold)
{
    // Initialize mask
    QImage mask(m_img.size(), QImage::Format_Indexed8);
    mask.setColor( 0, qRgba(0,0,0,0));  // Place holder - replaced in blinker/applyMask
    mask.setColor( 1, qRgba(0,0,0,0));  // Transparent

    // Find targets within threshold of target
    if ((m_img.format() == QImage::Format_RGB32) || (m_img.format() == QImage::Format_ARGB32))
    {
        int red = qRed(target);
        int grn = qGreen(target);
        int blu = qBlue(target);

        // Scan through page seeking matches
        for(int i=0; i<m_img.height(); i++)
        {
            QRgb *srcPtr = (QRgb *)m_img.scanLine(i);
            uchar *maskPtr = reinterpret_cast<uchar*>(mask.scanLine(i));
            for(int j=0; j<m_img.width(); j++)
            {
                QRgb val = *srcPtr++;
                int max = abs(red - qRed(val));
                int tmp = abs(grn - qGreen(val));
                if (tmp > max)
                    max = tmp;
                tmp = abs(blu - qBlue(val));
                if (tmp > max)
                    max = tmp;
                if (max > threshold)
                    *maskPtr++ = 1;
                else
                    *maskPtr++ = 0;
            }
        }
    }
    else if (m_img.format() == QImage::Format_Grayscale8)
    {
        int pix = qRed(target);

        // Scan through page seeking matches
        for(int i=0; i<m_img.height(); i++)
        {
            uchar *srcPtr = m_img.scanLine(i);
            uchar *maskPtr = reinterpret_cast<uchar*>(mask.scanLine(i));
            for(int j=0; j<m_img.width(); j++)
            {
                int val = *srcPtr++;
                int max = abs(pix - val);
                if (max > threshold)
                    *maskPtr++ = 1;
                else
                    *maskPtr++ = 0;
            }
        }
    }
    else
        return QImage();
    return mask;
}

//
// Select all colorful pixels
//
QImage Page::deColor(int threshold)
{
    // Initialize mask
    QImage mask(m_img.size(), QImage::Format_Indexed8);
    mask.setColor( 0, qRgba(0,0,0,0));  // Place holder - replaced in blinker/applyMask
    mask.setColor( 1, qRgba(0,0,0,0));  // Transparent

    // Find targets within threshold of target
    if ((m_img.format() == QImage::Format_RGB32) || (m_img.format() == QImage::Format_ARGB32))
    {
        int t = threshold * 2;
        // Scan through page seeking matches
        for(int i=0; i<m_img.height(); i++)
        {
            QRgb *srcPtr = (QRgb *)m_img.scanLine(i);
            uchar *maskPtr = reinterpret_cast<uchar*>(mask.scanLine(i));
            for(int j=0; j<m_img.width(); j++)
            {
                QRgb val = *srcPtr++;
                int red = qRed(val);
                int grn = qGreen(val);
                int blu = qBlue(val);
                // Calculate distance between pixel and gray line (0,0,0)->(255,255,255)
                //
                // d = norm(cross(p2-p1, p1-p0)) / norm(p2-p1)
                // where:
                // p0 = [r,g,b]
                // p1 = [0,0,0]
                // p2 = [1,1,1]
                // and
                // norm = sqrt(x^2 + y^2 + z^2)
                // cross(a, b) = [ ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx ]
                //
                // Simplifying:
                // d = norm(cross(p2, -p0)) / sqrt(3)
                //
                // Expanding:
                // d = norm( [ p2[y] * -p0[z] - p2[z] * -p0[y], p2[z] * -p0[x] - p2[x] * -p0[z], p2[x] * -p0[y] - p2[y] * -p0[x] ] ) / sqrt(3)
                //
                // Since all p2 is '1':
                // d = norm( [ -p0[z] - -p0[y], -p0[x] - -p0[z], -p0[y] - -p0[x] ] ) / sqrt(3)
                //
                // Simplify:
                // d = norm( [ p0[y] - p0[z], p0[z] - p0[x], p0[x] - p0[y] ] ) / sqrt(3)
                //
                // Finally:
                // d = norm( [ g - b, b - r, r - g ] ) / sqrt(3)
                //
                float d = (grn - blu)*(grn - blu) + (blu - red)*(blu - red) + (red - grn)*(red - grn);
                d = sqrt(d) / 1.732;
                if ( d < t )
                    *maskPtr++ = 1;
                else
                    *maskPtr++ = 0;
                //
                //if ((red * 2 > grn + blu + t) || (grn * 2 > red + blu + t) || (blu * 2 > red + grn + t))
                //    *maskPtr++ = 0;
                //else
                //    *maskPtr++ = 1;
            }
        }
    }
    return mask;
}

//
// Run this in a separate thread to keep from blocking the UI
//
QImage Page::despeckle(int blobSize, bool invert, int *blobs)
{
    // Invert for devoid
    QImage img = m_img;
    if (invert)
        img.invertPixels(QImage::InvertRgb);

    // Convert to grayscale
    if (img.format() != QImage::Format_Grayscale8)
        img = img.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat = QImage2OCV(img);

    // Make B&W with background black
    cv::Mat bw;
    cv::threshold(mat, bw, 250, 255, cv::THRESH_BINARY_INV);

    // Find blobs
    cv::Mat stats, centroids, labelImg;
    int nLabels = cv::connectedComponentsWithStats(bw, labelImg, stats, centroids, 4, CV_32S);

    // Initialize mask
    QImage mask(img.size(), QImage::Format_Indexed8);
    mask.setColor( 0, qRgba(0,0,0,0));  // Place holder - replaced in blinker/applyMask
    mask.setColor( 1, qRgba(0,0,0,0));  // Transparent
    mask.fill(1);

    // Build mask of blobs smaller than limit
    int cnt = 0;
    for(int idx=1; idx<nLabels; idx++)
    {
        // Check if this blob is small enough
        if (stats.at<int>(idx, cv::CC_STAT_AREA) <= blobSize)
        {
            int top = stats.at<int>(idx, cv::CC_STAT_TOP);
            int bot = stats.at<int>(idx, cv::CC_STAT_TOP) + stats.at<int>(idx, cv::CC_STAT_HEIGHT);
            int left = stats.at<int>(idx, cv::CC_STAT_LEFT);
            int right = stats.at<int>(idx, cv::CC_STAT_LEFT) + stats.at<int>(idx, cv::CC_STAT_WIDTH);

            // Sweep the enclosing rectangle, setting pixels in the mask
            for (int row=top; row<bot; row++)
            {
                uchar *maskPtr = reinterpret_cast<uchar*>(mask.scanLine(row));
                for (int col=left; col<right; col++)
                {
                    if (labelImg.at<int>(row,col) == idx)
                        maskPtr[col] = 0;
                }
            }
            cnt++;
        }
    }
    if (blobs != nullptr)
        *blobs = cnt;
    return mask;
}

//
// Select adjacent pixels near the cursor's color
//
QImage Page::floodFill(QPoint loc, int threshold)
{
    // Upconvert mono images
    QImage img;
    if (m_img.format() == QImage::Format_Mono)
        img = m_img.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
    else
        img = m_img;

    // Convert to OpenCV
    cv::Mat orig = QImage2OCV(img);
    if ((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32))
        cv::cvtColor(orig, orig, cv::COLOR_RGBA2RGB);   // floodfill doesn't work with alpha channel

    // Make all zero mask
    cv::Mat floodMask = cv::Mat::zeros(img.height() + 2, img.width() + 2, CV_8UC1);

    // Fill adjacent pixels
    cv::Point ref(loc.x(), loc.y());
    cv::Rect region;
    cv::Scalar thresh(threshold, threshold, threshold);
    int flags = 8 | (255 << 8 ) | cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY;
    cv::floodFill(orig, floodMask, ref, 0, &region, thresh, thresh, flags);

    // Convert mask back
    QImage tmp = OCV2QImage(floodMask);
    tmp = tmp.copy(1, 1, tmp.width() - 2, tmp.height() - 2);

    // Initialize mask
    QImage mask(img.size(), QImage::Format_Indexed8);
    mask.setColor( 0, qRgba(0,0,0,0));  // Place holder - replaced in blinker/applyMask
    mask.setColor( 1, qRgba(0,0,0,0));  // Transparent

    // Scan through page seeking matches
    for(int i=0; i<tmp.height(); i++)
    {
        uchar *srcPtr = tmp.scanLine(i);
        uchar *maskPtr = reinterpret_cast<uchar*>(mask.scanLine(i));
        for(int j=0; j<tmp.width(); j++)
        {
            if (*srcPtr++ <= 128)
                *maskPtr++ = 1;
            else
                *maskPtr++ = 0;
        }
    }
    return mask;
}

//
// Paint the mask onto the image
//
void Page::applyMask(QImage mask, QColor color)
{
    mask.setColor( 0, color.rgba() );
    QPainter p(&m_img);
    p.drawImage(QPoint(0,0), mask);
    p.end();
}

//
// Rotate the image by a small amount
//
QImage Page::deskew(float angle)
{
    // Run this in a thread to avoid lagging the UI
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QFuture<QImage> future = QtConcurrent::run(&Page::deskewThread, this, angle);
#else
    QFuture<QImage> future = QtConcurrent::run(this, &Page::deskewThread, angle);
#endif
    while (!future.isFinished())
    {
        QApplication::processEvents();
        QThread::msleep(1); //yield
    }
    future.waitForFinished();
    return future.result();
}

QImage Page::deskewThread(float angle)
{
    QTransform tmat = QTransform().rotate(angle);
    return m_img.transformed(tmat, Qt::SmoothTransformation);
}

//
// Calculate deskew angle
//
float Page::calcDeskew()
{
    QImage tmpImage = m_img;
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
    l_float32 angle, conf;
    float retval;
    if (pixFindSkew(pixS, &angle, &conf))
        retval = 0.0;
    else
        retval = angle;
    pixDestroy(&pixS);
    return retval;
}

//
// Apply the deskew image
//
void Page::applyDeskew(QImage img)
{
    // Paint the deskew image rotated about the center
    // Note: This code is identical to paintEvent
    QPainter p(&m_img);
    QRect rect(img.rect());
    rect.moveCenter(m_img.rect().center());
    p.drawImage(rect.topLeft(), img);
    p.end();
}

//
// Center image in page
//
void Page::doCenter(QColor bg)
{
    // Convert to grayscale
    QImage img;
    if (m_img.format() != QImage::Format_Grayscale8)
        img = m_img.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
    else
        img = m_img;

    // Convert to openCV
    cv::Mat orig = QImage2OCV(img);

    // Invert
    cv::Mat inverted;
    cv::bitwise_not(orig, inverted);

    // Calculate bounding box
    cv::Rect box = cv::boundingRect(inverted);

    // Calculate margins
    int left = (m_img.width() - box.width) / 2 - box.x;
    int top = (m_img.height() - box.height) / 2 - box.y;

    // Paint image onto m_img with calculated offset
    QImage tmp = m_img;
    QPainter p(&m_img);
    p.fillRect(m_img.rect(), bg);
    p.drawImage(QPoint(left, top), tmp);
    p.end();
}

//
// Convert current image to grayscale
//
void Page::toGrayscale()
{
    m_img = m_img.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
}

//
// Convert to binary
//
void Page::toBinary(bool adaptive, int blur, int kernel)
{
    // Run this in a thread to avoid lagging the UI
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QFuture<void> future = QtConcurrent::run(&Page::toBinaryThread, this, adaptive, blur, kernel);
#else
    QFuture<void> future = QtConcurrent::run(this, &Page::toBinaryThread, adaptive, blur, kernel);
#endif
    while (!future.isFinished())
    {
        QApplication::processEvents();
        QThread::msleep(1); //yield
    }
    future.waitForFinished();
}

//
// Convert to binary thread
//
void Page::toBinaryThread(bool adaptive, int blur, int kernel)
{
    // Convert to grayscale
    QImage img = m_img.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);

    // Convert to OpenCV
    cv::Mat mat = QImage2OCV(img);

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
    img = OCV2QImage(mat);
    img = img.convertToFormat(QImage::Format_Mono, Qt::MonoOnly|Qt::ThresholdDither|Qt::AvoidDither);

    // Copy metadata
    img.setDotsPerMeterX(m_img.dotsPerMeterX());
    img.setDotsPerMeterY(m_img.dotsPerMeterY());
    for (const auto& i : m_img.textKeys())
        img.setText(i, m_img.text(i));

    m_img = img;
}

//
// Convert current image to dithered binary
//
void Page::toDithered()
{
    m_img = m_img.convertToFormat(QImage::Format_Mono, Qt::MonoOnly|Qt::DiffuseDither);
}
