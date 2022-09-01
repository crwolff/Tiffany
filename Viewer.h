#ifndef VIEWER_H
#define VIEWER_H

#include <QWidget>
#include <QColor>
#include <QImage>
#include <QListWidget>
#include <QScrollArea>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QColor foregroundColor = Qt::black;
    QColor backgroundColor = Qt::white;
    float scaleBase = 1.0;
    float scaleFactor = 1.0;

public slots:
    void imageSelected(QListWidgetItem *curr, QListWidgetItem *prev);
    void pointerMode();
    void pencilMode();
    void eraserMode();
    void areaFillMode();
    void setBrush_1();
    void setBrush_4();
    void setBrush_8();
    void setBrush_12();
    void undoEdit();
    void redoEdit();
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void fitWidth();
    void fitHeight();
    void fillWindow();

signals:
    void zoomSig();
    void imageChangedSig();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    void setTransform();
    void drawLine(QPoint start, QPoint finish, QColor color);
    void fillArea(QRect rect);
    void zoomArea(QRect rect);
    void flushEdits();
    void pushImage();
    void updateScrollBars();
    void adjustScrollBars(float factor);
    bool measureAll(int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH);

    QListWidgetItem *currListItem = NULL;
    QImage currImage;
    QScrollArea *scrollArea = NULL;
    QTransform currTransform, currInverse;
    QString leftMode = "Pointer";
    qreal brushSize = 1.0;
    QRubberBand *rubberBand = NULL;
    QPoint origin;
    bool drawing = false;
};

#endif
