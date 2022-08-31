#ifndef COLORQTOOLBUTTON_H
#define COLORQTOOLBUTTON_H

#include <QWidget>
#include <QToolButton>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>

class ColorQToolButton : public QToolButton
{
    Q_OBJECT

public:
    ColorQToolButton(QWidget * parent = NULL);
    ~ColorQToolButton();
    void setIcon(QColor foregroundColor, QColor backgroundColor);
    QString mode = "Foreground";

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
};

#endif
