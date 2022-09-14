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
//    PageData(const PageData &p) : QImage(p) { m_changes = p.m_changes; m_rotation = p.m_rotation; }
    ~PageData() {}

    inline int changes() { return m_changes; }
    inline void setChanges(int val) { m_changes = val; }
    inline int rotation() { return m_rotation; }
    inline void setRotation(int val) { m_rotation = val & 3; }
    inline bool modified() { return (m_changes != 0) || (m_rotation != 0); }

private:
    void _init() { m_changes = 0; m_rotation = 0; }
    int m_changes;
    int m_rotation;
};

Q_DECLARE_METATYPE(PageData)
#endif // PAGEDATA_H
