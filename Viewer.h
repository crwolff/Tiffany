#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

protected:
    void paintEvent(QPaintEvent * event);

};

#endif
