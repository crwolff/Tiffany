#ifndef VIEWER_H
#define VIEWER_H

#include "Page.h"
#include <QImage>
#include <QListWidget>
#include <QScrollArea>
#include <QTimer>
#include <QWidget>

class Viewer : public QWidget
{
    Q_OBJECT

public:
    Viewer(QWidget * parent = NULL);
    ~Viewer();

    QTimer *blinkTimer = new QTimer();
    enum LeftMode { Select, Pencil, Eraser, ColorSelect, FloodFill };
    enum RightMode { Idle, Zoom, Pan };
    enum MatchCode { None, Exact, Shifted, Ctrled };

public slots:
    void changePage(QListWidgetItem *curr);
    void updatePage(bool updateZoom);
    void setTool(LeftMode tool);
    void resetTools();

    void blinker();
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void fitWidth();
    void fitHeight();
    void fillWindow();

signals:
    void updateIconSig();
    void zoomSig();

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    MatchCode keyMatches(QKeyEvent *event, QKeySequence::StandardKey matchKey);
    void drawLine(QPoint start, QPoint finish, QColor color);
    void drawDot(QPoint loc, QColor color);
    void fillArea(QRect rect, QColor color, bool outside);
    void doCopy(QRect box);
    void setupPaste();
    void doPaste(bool transparent);
    QPoint pasteLocator(QPoint mouse, bool optimize);
    QImage floodFill(QPoint loc, int threshold);

    void zoomArea(QRect rect);
    void zoomWheel(QPoint pos, float factor);
    void updateScrollBars();
    void adjustScrollBars(float factor);
    bool measureAll(Page &page, int &scrollBarSize, int &viewW, int &viewH, int &imageW, int &imageH);
    void setScaleFactor(float val);

    QPoint leftOrigin;
    QPoint rightOrigin;
    QCursor lastCursor;
    QListWidgetItem *currItem = nullptr;
    Page currPage;
    float scaleFactor;
    QTransform pageToScrn;
    QTransform scrnToPage;
    QTransform scrnToPageOffs;
    QScrollArea *scrollArea = NULL;
    QRubberBand *leftBand = new QRubberBand(QRubberBand::Rectangle, this);
    QPoint LMRBstart;   // Left mouse rubberBand start
    QPoint LMRBend;
    QRubberBand *rightBand = new QRubberBand(QRubberBand::Rectangle, this);
    QPoint RMRBstart;
    QPoint RMRBend;
    QImage logo;
    LeftMode leftMode = Select;
    RightMode rightMode = Idle;

    QCursor PencilCursor;
    QCursor Pencil180Cursor;
    QCursor DropperCursor;
    bool pencil180;
    bool shiftPencil = false;
    QPoint drawLoc;
    QColor currColor;

    bool pasting = false;
    QPoint pasteLoc;
    QImage copyImage;
    QList<QImage> copyImageList;

    QImage pageMask;
};

#endif
