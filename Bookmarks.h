#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QWidget>
#include <QListWidget>
#include "PageData.h"

class Bookmarks : public QListWidget
{
    Q_OBJECT

public:
    Bookmarks(QWidget * parent = NULL);
    ~Bookmarks();

public slots:
    void openFiles();
    void insertFiles();
    void replaceFiles();
    void toGrayscale();
    void toBinary();
    void saveFiles();
    void saveToDir();
    void selectEven();
    void selectOdd();
    void deleteSelection();
    void rotateCW();
    void rotateCCW();
    void rotate180();
    void updateIcon();

signals:
    void progressSig(QString descr, int val);

private:
    void readFiles(QString cmd);
    void rotateSelection(int val);
    QIcon makeIcon(PageData &image, bool flag);
};

#endif
