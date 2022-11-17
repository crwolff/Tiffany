#include <leptonica/allheaders.h>
#include <QDebug>
#include <QImage>

PIX* QImage2PIX(QImage &img);
QImage PIX2QImage(PIX *pix);
QImage PIX2QImage(PIX *pix, QImage &ref);
