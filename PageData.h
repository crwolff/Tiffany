// PageData.h

#ifndef PAGEDATA_H
#define PAGEDATA_H
#include <QMetaType>
#include <QImage>
#include <QString>
#include <QDebug>

class PageData : public QImage
{
public:
    PageData() : QImage() { _init(); }
    PageData(const QString &fileName, const char *format = nullptr) : QImage(fileName, format) { _init(); }
    PageData(const QImage &image) : QImage(image) { _init(); }
    ~PageData() {}

    inline int changes() { return m_changes; }
    inline void setChanges(int val) { m_changes = val; }
    inline qreal rotation() { return m_rotation; }
    inline void setRotation(qreal val) { m_rotation = (val >= 360.0) ? val - 360.0 : val; }
    inline bool modified() { return (m_changes != 0) || (m_rotation != 0.0); }

    // Copy metadata from old page to new
    inline void copyOtherData(PageData &old)
    {
        m_changes = old.m_changes;
        m_rotation = old.m_rotation;
    }

private:
    void _init()
    {
        m_changes = 0;
        m_rotation = 0.0;
    }
    int m_changes;
    qreal m_rotation;
};

Q_DECLARE_METATYPE(PageData)
#endif // PAGEDATA_H
