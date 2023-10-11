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
    PageData() : QImage() {}
    PageData(const QString &fileName, const char *format = nullptr) : QImage(fileName, format) {}
    PageData(const QImage &image) : QImage(image) {}
    ~PageData() {}
    inline bool modified() { return false; }
};

Q_DECLARE_METATYPE(PageData)
#endif // PAGEDATA_H
