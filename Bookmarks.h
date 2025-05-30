#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include "Page.h"
#include <QEnterEvent>
#include <QImage>
#include <QListWidget>
#include <QWidget>

class Bookmarks : public QListWidget
{
    Q_OBJECT

public:
    Bookmarks(QWidget * parent = NULL);
    ~Bookmarks();

public slots:
    void itemSelectionChanged();
    void openFiles();
    void insertFiles();
    void replaceFiles();
    void saveFiles();
    void saveToDir();
    bool anyModified();
    void selectEven();
    void selectOdd();
    void selectModified();
    void deleteSelection();
    void blankPage();
    void removeBG();
    void despeckle();
    void devoid();
    void deskew();
    void toGrayscale();
    void toBinary();
    void toAdaptive();
    void toDithered();
    void centerPage();
    void rotateCW();
    void rotateCCW();
    void rotate180();
    void mirrorHoriz();
    void mirrorVert();
    void updateIcon();
    void undoEdit();
    void redoEdit();

signals:
    void changePageSig(QListWidgetItem* curr);
    void updatePageSig(bool updateZoom);
    void progressSig(QString descr, int val);

private:
    void readFiles(QString cmd);
    bool saveCommon(QListWidgetItem* itemPtr, QString &fileName, QString &backupName);
    void rotateSelection(int val);
    void mirrorSelection(int dir);
    QIcon makeIcon(QImage &image, bool flag);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
};

#endif
