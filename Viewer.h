#ifndef VIEWER_H
#define VIEWER_H

#include "PageData.h"
#include <QWidget>
#include <QColor>
#include <QImage>
#include <QListWidget>
#include <QScrollArea>
#include <QTimer>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QColor foregroundColor = Qt::black;
    QColor backgroundColor = Qt::white;
    QTimer *blinkTimer = new QTimer();

public slots:
    void blinker();
    void imageSelected(QListWidgetItem *curr, QListWidgetItem *prev);
    void updateViewer();
    void pointerMode();
    void dropperMode();
    void pencilMode();
    void eraserMode();
    void deskewMode();
    void despeckleMode();
    void setBrush_1();
    void setBrush_4();
    void setBrush_8();
    void setBrush_12();
    void blankPage();
    void setThreshold(int val);
    void colorSelect();
    void setDeskew(double val);
    void deskew();
    void setDespeckle(int val);
    void despeckle();
    void undoEdit();
    void redoEdit();
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void fitWidth();
    void fitHeight();
    void fillWindow();

signals:
    void zoomSig(float scale);
    void imageChangedSig();

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    void drawLine(QPoint start, QPoint finish, QColor color);
    void fillArea(QRect rect, bool outside);
    void applyMask(QImage &mask, bool flag);
    void applyDeskew();
    void copySelection();
    void pasteSelection();
    void zoomArea(QRect rect);
    void zoomWheel(QPoint pos, float factor);
    void pushImage();
    void updateScrollBars();
    void adjustScrollBars(float factor);
    bool measureAll(PageData &page, int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH);

    QListWidgetItem *currListItem = NULL;
    PageData currPage;
    QImage currMask;
    QImage deskewImg;
    bool blink = false;
    float scaleFactor = 1.0;
    float scaleBase = 1.0;
    QScrollArea *scrollArea = NULL;
    qreal brushSize = 1.0;
    QRubberBand *rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    QPoint origin;
    bool pasting = false;
    QPoint pasteLoc;
    QImage copyImage;
    QCursor PencilCursor;
    QCursor DropperCursor;
    QCursor DespeckleCursor;
    QImage logo;
    int dropperThreshold = 20;
    double deskewAngle = 0.0;
    int despeckleArea = 50;
    QPointF dropperLoc = QPointF(0,0);

    enum LeftMode { Select, ColorSelect, Pencil, Eraser, Deskew, Despeckle };
    enum RightMode { Idle, Zoom, Pan };
    LeftMode leftMode = Select;
    RightMode rightMode = Idle;
    int gridOffsetX = 0;
    int gridOffsetY = 0;
};

#endif
