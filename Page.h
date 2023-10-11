// Page.h

#ifndef PAGE_H
#define PAGE_H
#include <QImage>
#include <QMetaType>

class Page : public QImage
{
public:
    // Constructors
    Page();
    Page(const QString &fileName, const char *format = nullptr);
    Page(const QImage &image);
    ~Page();

    // Methods
    bool modified();
};

Q_DECLARE_METATYPE(Page)
#endif // PAGE_H
