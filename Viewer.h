#ifndef VIEWER_H
#define VIEWER_H

#include "PageData.h"
#include <QWidget>
#include <QClipboard>
#include <QColor>
#include <QFont>
#include <QImage>
#include <QListWidget>
#include <QMutex>
#include <QScrollArea>
#include <QTimer>
#include <opencv2/core/types.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QColor foregroundColor = Qt::black;
    QColor backgroundColor = Qt::white;
    QTimer *blinkTimer = new QTimer();
    int dropperThreshold;
    int floodThreshold;
    double deskewAngle;
    int despeckleArea;
    qreal brushSize;
    int blurRadius;
    int kernelSize;
    QFont textFont;
    enum LeftMode { Select, ColorSelect, FloodFill, Pencil, Eraser, Deskew, Despeckle };
    enum RightMode { Idle, Zoom, Pan };

public slots:
    void blinker();
    void imageSelected(QListWidgetItem *curr, QListWidgetItem *prev);
    void updateViewer();
    void setTool(LeftMode tool);
    void setBrush(qreal sz);
    void blankPage();
    void setDropperThreshold(int val);
    void colorSelect();
    void setFloodThreshold(int val);
    void floodFill();
    void setDeskew(double val);
    void deskew();
    void deskewThread();
    void setDespeckle(int val);
    void despeckle();
    void despeckleThread();
    void toGrayscale();
    void setBlurRadius(int val);
    void setKernelSize(int val);
    void toBinary();
    void toAdaptive();
    void binarization(bool adaptive);
    void binThread(bool adaptive);
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
    void setDeskewWidget(float val);

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
    QMutex mutex;
    void drawLine(QPoint start, QPoint finish, QColor color);
    void drawDot(QPoint loc, QColor color);
    void fillArea(QRect rect, bool outside);
    void applyMask(QImage &mask, bool flag);
    void applyDeskew();
    void copySelection();
    QPointF pasteOptimizer(qreal &imgW, qreal &imgH, QPointF &loc);
    void pasteSelection(bool ctrl);
    void zoomArea(QRect rect);
    void zoomWheel(QPoint pos, float factor);
    void regionOCR();
    void doWarp();
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
    QRubberBand *rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    QPoint origin;
    bool pasting = false;
    QPoint pasteLoc;
    bool pasteCtrl;
    QImage copyImage;
    QList<QImage> copyImageList;
    QCursor PencilCursor;
    QCursor Pencil180Cursor;
    bool pencil180 = false;
    QCursor DropperCursor;
    QCursor DespeckleCursor;
    QImage logo;
    QPointF dropperLoc = QPointF(0,0);
    QPointF floodLoc = QPointF(0,0);
    QColor currColor = Qt::black;
    bool shiftPencil = false;
    QPoint drawLoc;
    bool binMode = false;
    tesseract::TessBaseAPI *tessApi = nullptr;
    QClipboard *clipboard = nullptr;
    int warpCount = 0;
    std::vector<cv::Point2f> warpCorner = std::vector<cv::Point2f>(4,cv::Point2f());
    std::vector<cv::Point2f> warpBefore = std::vector<cv::Point2f>(4,cv::Point2f());
    std::vector<cv::Point2f> warpAfter = std::vector<cv::Point2f>(4,cv::Point2f());

    LeftMode leftMode = Select;
    RightMode rightMode = Idle;
    int gridOffsetX = 0;
    int gridOffsetY = 0;
};

#endif
