// UndoBuffer.h

#ifndef UNDOBUFFER_H
#define UNDOBUFFER_H
#include "PageData.h"
#include <QMetaType>
#include <QDebug>

#define MAX_UNDO 5

class UndoBuffer
{
public:
    UndoBuffer() {}
    ~UndoBuffer() {}

    // Flush undo buffer
    void flush()
    {
        while (!m_undoState.isEmpty())
            m_undoState.removeFirst();
        while (!m_redoState.isEmpty())
            m_redoState.removeFirst();
    }

    // Add current page to undo list
    void push(PageData &page)
    {
        if (page.isNull())
            return;
        m_undoState.insert(0, page);
        if (m_undoState.count() > MAX_UNDO)
            m_undoState.removeLast();
        while (!m_redoState.isEmpty())
            m_redoState.removeFirst();
    }

    // Rollback one change
    PageData undo(PageData &page)
    {
        if (page.isNull())
            return page;
        if (m_undoState.count() > 0)
        {
            m_redoState.insert(0, page);
            return m_undoState.takeFirst();
        }
        else
            return page;
    }

    // Reapply previous undo
    PageData redo(PageData &page)
    {
        if (page.isNull())
            return page;
        if (m_redoState.count() > 0)
        {
            m_undoState.insert(0, page);
            return m_redoState.takeFirst();
        }
        else
            return page;
    }

    // Peek at the last page saved
    const PageData peek(PageData &page)
    {
        if (m_undoState.count() > 0)
            return m_undoState.first();
        else
            return page;
    }

private:
    QList<PageData> m_undoState;
    QList<PageData> m_redoState;
};

Q_DECLARE_METATYPE(UndoBuffer)
#endif // UNDOBUFFER_H
