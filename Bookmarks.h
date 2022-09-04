#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include <QWidget>
#include <QListWidget>

class Bookmarks : public QListWidget
{
    Q_OBJECT

public:
    Bookmarks(QWidget * parent = NULL);
    ~Bookmarks();

public slots:
    void readFiles();
    void toGrayscale();
    void toBinary();
    void saveFiles();
    void saveAsFiles();
    void createTIFF();
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
    void rotateSelection(int val);
    QIcon makeIcon(QImage &image, bool flag);
};

#endif
