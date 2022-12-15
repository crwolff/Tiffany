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
    inline int rotation() { return m_rotation; }
    inline void setRotation(int val) { m_rotation = val & 3; }
    inline int mirrors() { return m_mirrors; }
    inline void setMirrors(int val) { m_mirrors = val; }
    inline bool modified() { return (m_changes != 0) || (m_rotation != 0) || (m_mirrors != 0); }

    // Copy metadata from old page to new
    inline void copyOtherData(PageData &old)
    {
        m_changes = old.m_changes;
        m_rotation = old.m_rotation;
        m_mirrors = old.m_mirrors;
    }

private:
    void _init()
    {
        m_changes = 0;
        m_rotation = 0;
        m_mirrors = 0;
    }
    int m_changes;
    int m_rotation;
    int m_mirrors;
};

Q_DECLARE_METATYPE(PageData)
#endif // PAGEDATA_H
