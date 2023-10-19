// Page.cpp

#include "Config.h"
#include "Page.h"
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
// Paint the mask onto the image
//
void Page::applyMask(QImage mask, QColor color)
{
    mask.setColor( 0, color.rgba() );
    QPainter p(&m_img);
    p.drawImage(QPoint(0,0), mask);
    p.end();
}
