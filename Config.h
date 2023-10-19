//
// Global configuration
//
#ifndef CONFIG_H
#define CONFIG_H

#include <QColor>
#include <QFont>
#include <QPointF>

namespace Config
{
    extern int bgRemoveThreshold;
    extern int dropperThreshold;
    extern int floodThreshold;
    extern double deskewAngle;
    extern int despeckleArea;
    extern int devoidArea;
    extern int brushSize;
    extern int blurRadius;
    extern int kernelSize;
    extern QFont textFont;
    extern QPointF locate1;
    extern QPointF locate2;

    extern QColor fgColor;
    extern QColor bgColor;

    void LoadConfig();
    void SaveConfig();
}

#endif
