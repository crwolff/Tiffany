// Page.cpp

#include "Page.h"
#include <QDebug>

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
// Report if image has changed
//
bool Page::modified()
{
    return false;
}
