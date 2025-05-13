//
// Global configuration
//
#include "Config.h"
#include <QSettings>

namespace Config
{
    int bgRemoveThreshold;
    int dropperThreshold;
    int floodThreshold;
    double deskewAngle;
    int deColorDist;
    int despeckleArea;
    int devoidArea;
    int brushSize;
    int blurRadius;
    int adaptiveBlurRadius;
    int kernelSize;
    QFont textFont;
    QPointF locate1;
    QPointF locate2;

    QColor fgColor;
    QColor bgColor;
    bool multiPage;

    // Load settings file
    void LoadConfig()
    {
        QSettings settings;

        bgRemoveThreshold = settings.value("bgRemoveThreshold", 40).toInt();
        dropperThreshold = settings.value("dropperThreshold", 40).toInt();
        floodThreshold = settings.value("floodThreshold", 50).toInt();
        deColorDist = settings.value("deColorDist", 20).toInt();
        despeckleArea = settings.value("despeckleArea", 20).toInt();
        devoidArea = settings.value("devoidArea", 20).toInt();
        brushSize = settings.value("brushSize", 1).toInt();
        if (brushSize >= 12)
            brushSize = 12;
        else if (brushSize >= 8)
            brushSize = 8;
        else if (brushSize >= 4)
            brushSize = 4;
        else
            brushSize = 1;
        blurRadius = settings.value("blurRadius", 5).toInt();
        if (blurRadius % 2 != 1)
            blurRadius++;
        adaptiveBlurRadius = settings.value("adaptiveBlurRadius", 5).toInt();
        if (adaptiveBlurRadius % 2 != 1)
            adaptiveBlurRadius++;
        kernelSize = settings.value("kernelSize", 23).toInt();
        if (kernelSize % 2 != 1)
            kernelSize++;
        QString font = settings.value("font", "Courier New,20,-1,5,50,0,0,0,0,0").toString();
        textFont.fromString(font);
        QString val1 = settings.value("locate1", "0,0").toString();
        locate1 = QPointF(val1.split(",")[0].toFloat(),val1.split(",")[1].toFloat());
        QString val2 = settings.value("locate2", "0,0").toString();
        locate2 = QPointF(val2.split(",")[0].toFloat(),val2.split(",")[1].toFloat());

        // Not loaded or saved
        deskewAngle = 0.0;
        fgColor = Qt::black;
        bgColor = Qt::white;
        multiPage = false;
    }

    // Save settings file
    void SaveConfig()
    {
        QSettings settings;
        settings.setValue("bgRemoveThreshold", bgRemoveThreshold);
        settings.setValue("dropperThreshold", dropperThreshold);
        settings.setValue("floodThreshold", floodThreshold);
        settings.setValue("deColorDist", deColorDist);
        settings.setValue("despeckleArea", despeckleArea);
        settings.setValue("devoidArea", devoidArea);
        settings.setValue("brushSize", brushSize);
        settings.setValue("blurRadius", blurRadius);
        settings.setValue("adaptiveBlurRadius", adaptiveBlurRadius);
        settings.setValue("kernelSize", kernelSize);
        settings.setValue("font", textFont.toString());
        settings.setValue("locate1", QStringLiteral("%1,%2").arg(locate1.x()).arg(locate1.y()));
        settings.setValue("locate2", QStringLiteral("%1,%2").arg(locate2.x()).arg(locate2.y()));
    }
}
