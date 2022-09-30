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
    void pointerMode();
    void pencilMode();
    void setBrush_1();
    void setBrush_4();
    void setBrush_8();
    void setBrush_12();
    void blankPage();
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
    float scaleFactor = 1.0;
    float scaleBase = 1.0;
    QScrollArea *scrollArea = NULL;
    QString leftMode = "Pointer";
    QString rightMode = "";
    qreal brushSize = 1.0;
    QRubberBand *rubberBand = NULL;
    QPoint origin;
    bool drawing = false;
    bool pasting = false;
    QPoint pasteLoc;
    QImage copyImage;
    QCursor PencilCursor;
    QCursor DropperCursor;
};

#endif
