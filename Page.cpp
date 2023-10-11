// Page.cpp

#include "Page.h"
#include <QImage>
#include <QDebug>

// Constuctors
Page::Page() : QImage()
{
}

Page::Page(const QString &fileName, const char *format) : QImage(fileName, format)
{
}

Page::Page(const QImage &image) : QImage(image)
{
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
