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
void Page::undo()
{
    if (m_undo.count() > 0)
    {
        m_redo.insert(0, m_img);
        m_img = m_undo.takeFirst();
        m_modified--;
    }
}

//
// Redo last undo
//
void Page::redo()
{
    if (m_redo.count() > 0)
    {
        m_undo.insert(0, m_img);
        m_img = m_redo.takeFirst();
        m_modified++;
    }
}
