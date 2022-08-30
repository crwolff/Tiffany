#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include <QColor>
#include <QListWidget>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QColor foregroundColor = Qt::black;
    QColor backgroundColor = Qt::white;

public slots:
    void imageSelected(QListWidgetItem *curr, QListWidgetItem *prev);
    void pointerMode();
    void pencilMode();
    void eraserMode();
    void areaFillMode();
    void setBrush();
    void undoEdit();
    void redoEdit();
    void zoomIn();
    void zoomOut();
    void zoomArea();
    void fitToWindow();
    void fitWidth();
    void fitHeight();
    void fillWindow();

signals:

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    void setTransform();
    void drawLine(QPoint *start, QPoint *finish, QColor color);
    void fillArea(QRect *rect);
    void flushEdits();
    void pushImage();
    void updateScrollBars();
    void adjustScrollBars(float factor);
    bool measureAll();
};

#endif
