//
// Global configuration
//
#ifndef CONFIG_H
#define CONFIG_H

#include <QFont>

namespace Config
{
    extern int dropperThreshold;
    extern int floodThreshold;
    extern double deskewAngle;
    extern int despeckleArea;
    extern int devoidArea;
    extern int brushSize;
    extern int blurRadius;
    extern int kernelSize;
    extern QFont textFont;

    void LoadConfig();
    void SaveConfig();
}

#endif
