#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include "Page.h"
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
    void openFiles();
    void insertFiles();
    void replaceFiles();
    void saveFiles();
    void saveToDir();
    bool anyModified();
    void selectEven();
    void selectOdd();
    void deleteSelection();
    void updateIcon();

signals:
    void progressSig(QString descr, int val);

private:
    void readFiles(QString cmd);
    bool saveCommon(QListWidgetItem* itemPtr, QString &fileName, QString &backupName);
    QIcon makeIcon(Page &image, bool flag);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

#endif
