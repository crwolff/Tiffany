// Page.cpp

#include "Config.h"
#include "Page.h"
#include "Utils/QImage2OCV.h"
#include <QDebug>
#include <QPainter>

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
// Select all pixels near the cursor's color
//
QImage Page::colorSelect(QRgb target, int threshold)
{
    // Initialize mask
    QImage mask(m_img.size(), QImage::Format_Indexed8);
    mask.setColor( 0, Config::fgColor.rgba() );
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
// Run this in a separate thread to keep from blocking the UI
//
QImage Page::despeckle(int blobSize, bool invert)
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
    mask.setColor( 0, Config::fgColor.rgba() );
    mask.setColor( 1, qRgba(0,0,0,0));  // Transparent
    mask.fill(1);

    // Build mask of blobs smaller than limit
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
