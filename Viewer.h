#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include <QColor>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QColor foregroundColor = Qt::black;
    QColor backgroundColor = Qt::white;

protected:
    void paintEvent(QPaintEvent * event) override;
};

#endif
