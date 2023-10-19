// Page.h

#ifndef PAGE_H
#define PAGE_H
#include <QImage>
#include <QMetaType>

class Page
{
public:
    // Constructors
    Page();
    Page(const QString &fileName, const char *format = nullptr);
    Page(const QImage &image);
    ~Page();

    // Methods
    bool modified();
    void flush();
    void push();
    bool undo();
    bool redo();
    QImage colorSelect(QRgb target);
    void applyMask(QImage mask, QColor color);

    // Flag if image was changed
    unsigned int m_modified = 0;

    // View position data
    float scaleFactor = 0.0;
    int horizontalScroll = 0;
    int verticalScroll = 0;

    // The main image
    QImage m_img;

    // Undo buffers
#define MAX_UNDO 8
    QList<QImage> m_undo;
    QList<QImage> m_redo;
};

Q_DECLARE_METATYPE(Page)
#endif // PAGE_H
