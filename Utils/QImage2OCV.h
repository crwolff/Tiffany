// QImage2OCV.h

#ifndef QIMAGE2OCV_H
#define QIMAGE2OCV_H

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>
#include <QDebug>
#include <QImage>

cv::Mat QImage2OCV(QImage &img);
QImage OCV2QImage(cv::Mat &inMat);
QImage OCV2QImage(cv::Mat &inMat, QImage &ref);
#endif
