#include "QImage2PIX.h"

//
// QImage little-endian formats:
//     Format_Mono        1 bit/pixel packed MSB first
//     Format_Grayscale8  8 bit/pixel
//     Format_RGB888      3 byte/pixel R G B
//     Format_ARGB32      4 byte/pixel B G R A
//     Format_RGB32       4 byte/pixel B G R (255)
//

//
// Create a leptonica PIX from a qimage
//      Works for color and grayscale
//
PIX* QImage2PIX(QImage &img)
{
    // Nothing to convert
    if (img.isNull())
        return nullptr;

    // Swap RGB to BGR if required
    QImage tmp;
    if (img.format() == QImage::Format_RGB888)
        tmp = img.rgbSwapped();
    else
        tmp = img;

    // Extract sizes
    int w = tmp.width();
    int h = tmp.height();
    int d = tmp.depth();

    // Make target array
    PIX *pix = pixCreate(w, h, d);
    pixSetColormap(pix, nullptr);

    // How many bytes to copy
    int srcBpl = tmp.bytesPerLine();
    int dstBpl = 4 * pixGetWpl(pix);
    if (dstBpl < srcBpl)
    {
        qWarning() << "QImage2PIX: Destination line is too short";
        return nullptr;
    }

    // Scan lines can have more bytes than required for width, so
    // copy one line at a time
    for(int y = 0; y < h; y++)
    {
        uchar *srcPtr = tmp.scanLine(y);
        uchar *dstPtr = (uchar *)pix->data + y * dstBpl;
        memcpy(dstPtr, srcPtr, srcBpl);
        // Pad with zeros
        for(int x = 0; x < dstBpl - srcBpl; x++)
            dstPtr[srcBpl + x] = 0;
    }

    return pix;
}

//
// Create a QImage from a openCV matrix
//      Works for color and grayscale
//
QImage PIX2QImage(PIX *pix)
{
    QImage ref;
    return PIX2QImage(pix, ref);
}

QImage PIX2QImage(PIX *pix, QImage &ref)
{
    QImage img;

    // Copy metadata
    if (!ref.isNull())
    {
        img.setDotsPerMeterX(ref.dotsPerMeterX());
        img.setDotsPerMeterY(ref.dotsPerMeterY());
        for (const auto& i : ref.textKeys())
            img.setText(i, ref.text(i));
    }

    return img;
}
