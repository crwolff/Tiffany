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
    void readFiles(QString cmd);
    void toGrayscale();
    void toBinary();
    void saveFiles();
    void saveToDir();
    void selectEven();
    void selectOdd();
    void deleteSelection();
    void rotateSelection(qreal val);
    void updateIcon();

signals:
    void progressSig(QString descr, int val);

private:
    QIcon makeIcon(PageData &image, bool flag);
};

#endif
