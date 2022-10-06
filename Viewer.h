#ifndef VIEWER_H
#define VIEWER_H

#include "PageData.h"
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

public slots:
    void imageSelected(QListWidgetItem *curr, QListWidgetItem *prev);
    void updateViewer();
    void pointerMode();
    void dropperMode();
    void pencilMode();
    void setBrush_1();
    void setBrush_4();
    void setBrush_8();
    void setBrush_12();
    void blankPage();
    void setThreshold(int val);
    void colorSelect();
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
    QImage logo;
    int dropperThreshold = 20;
    QPointF dropperLoc = QPointF(0,0);

    enum LeftMode { Select, ColorSelect, Draw };
    enum RightMode { Idle, Zoom, Pan };
    LeftMode leftMode = Select;
    RightMode rightMode = Idle;
};

#endif
