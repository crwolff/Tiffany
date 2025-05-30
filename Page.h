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
    QImage peek();
    QImage colorSelect(QRgb target, int threshold);
    QImage deColor(int threshold);
    QImage despeckle(int blobSize, bool invert, int *blobs = nullptr);
    QImage floodFill(QPoint loc, int threshold);
    void applyMask(QImage mask, QColor color);
    QImage deskew(float angle);
    float calcDeskew();
    void applyDeskew(QImage img);
    void doCenter(QColor bg);
    void toGrayscale();
    void toBinary(bool adaptive, int blur, int kernel=1);
    void toDithered();

    // Flag if image was changed
    unsigned int m_modified = 0;

    // View position data
    float scaleFactor = 0.0;
    int horizontalScroll = 0;
    int verticalScroll = 0;

    // The main image
    QImage m_img;

private:
    QImage deskewThread(float angle);
    void toBinaryThread(bool adaptive, int blur, int kernel);

    // Undo buffers
#define MAX_UNDO 8
    QList<QImage> m_undo;
    QList<QImage> m_redo;
};

Q_DECLARE_METATYPE(Page)
#endif // PAGE_H
