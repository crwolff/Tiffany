#include "QImage2OCV.h"

//
// Create a openCV matrix from a qimage
//      Works for color and grayscale
//
cv::Mat QImage2OCV(QImage &img)
{
    int fmt;

    if (img.isNull())
        return cv::Mat();

    // Pick destination format
    switch (img.format())
    {
        case QImage::Format_Grayscale8:
            fmt = CV_8UC1;
            break;
        case QImage::Format_RGB888:
            fmt = CV_8UC3;
            break;
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
            fmt = CV_8UC4;
            break;
        default:
            qWarning() << "QImage2OCV: QImage type not handled in switch: " << img.format();
            return cv::Mat();
    }

    // Convert to matrix
    cv::Mat mat(img.height(), img.width(), fmt, img.bits(), img.bytesPerLine());
    if (fmt == CV_8UC3)
        cv::cvtColor(mat, mat, CV_RGB2BGR);
    return mat.clone();
}

//
// Create a QImage from a openCV matrix
//      Works for color and grayscale
//
QImage OCV2QImage(cv::Mat &mat)
{
    QImage::Format fmt;

    // Pick destination format
    switch (mat.type())
    {
        case CV_8UC4:
            fmt = QImage::Format_ARGB32;
            break;
        case CV_8UC3:
            fmt = QImage::Format_RGB888;
            break;
        case CV_8UC1:
            fmt = QImage::Format_Grayscale8;
            break;
        default:
            qWarning() << "OCV2QImage: cv::Mat image type not handled in switch: " << mat.type();
            return QImage();
    }

    // Convert to QImage
    QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), fmt);
    if (mat.type() == CV_8UC3)
        image = image.rgbSwapped();
    return image.copy();
}
