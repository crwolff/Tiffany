#ifndef VIEWER_H
#define VIEWER_H

#include "Page.h"
#include <QImage>
#include <QListWidget>
#include <QScrollArea>
#include <QWidget>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QColor foregroundColor = Qt::black;
    QColor backgroundColor = Qt::white;
    enum RightMode { Idle, Zoom, Pan };

public slots:
    void changePage(Page curr);
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void fitWidth();
    void fitHeight();
    void fillWindow();

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    void zoomArea(QRect rect);
    void zoomWheel(QPoint pos, float factor);
    void updateScrollBars();
    void adjustScrollBars(float factor);
    bool measureAll(Page &page, int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH);

    QPoint origin;
    QCursor LastCursor;
    Page currPage;
    float scaleFactor = 1.0;
    float scaleBase = 1.0;
    QScrollArea *scrollArea = NULL;
    QRubberBand *rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    QImage logo;
    RightMode rightMode = Idle;
};

#endif
