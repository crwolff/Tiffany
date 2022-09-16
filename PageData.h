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
    inline bool modified() { return (m_changes != 0) || (m_rotation != 0); }
    inline float scaleBase() const { return m_scaleBase; }
    inline void setScaleBase(float val) { m_scaleBase = val; }
    inline float scaleFactor() const { return m_scaleFactor; }
    inline void setScaleFactor(float val) { m_scaleFactor = (val > 10000) ? 10000 : val; }

private:
    void _init()
    {
        m_changes = 0;
        m_rotation = 0;
        m_scaleBase = 1.0;
        m_scaleFactor = 1.0;
    }
    int m_changes;
    int m_rotation;
    float m_scaleBase;
    float m_scaleFactor;
};

Q_DECLARE_METATYPE(PageData)
#endif // PAGEDATA_H
