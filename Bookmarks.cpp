#include "Bookmarks.h"
#include <QColor>
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPainter>

Bookmarks::Bookmarks(QWidget * parent) : QListWidget(parent)
{
}

Bookmarks::~Bookmarks()
{
}

//
// Load files into bookmark viewer
//
void Bookmarks::readFiles()
{
    QString whom = QObject::sender()->objectName();
    QString txt;

    // Determine button pressed
    if (whom == "openAct")
        txt = "Open Files";
    else if (whom == "insertAct")
        txt = "Insert Files";
    else if (whom == "replaceAct")
        txt = "Replace Files";
    else
        txt = "Huh?";

    // Popup file dialog
    QStringList filenames = QFileDialog::getOpenFileNames(this, txt, "", "Images (*.png)");
    if (filenames.isEmpty())
        return;

    // For replace or insert, get list of items
    QVector<int> rows;
    if ((whom == "insertAct") || (whom == "replaceAct"))
    {
        QList<QListWidgetItem*> items = selectedItems();
        foreach(QListWidgetItem* item, items)
            rows.append(item->text().toInt() - 1);
        if (rows.count() == 0)
        {
            QMessageBox::information(this, txt, "Insertion point must be selected");
            return;
        }
        std::sort(rows.begin(), rows.end());
    }
    else // openAct
        rows.append(count());

    // Add progress to status bar
    emit progressSig("Reading...", filenames.count());

    // Open each file in turn and add to listWidget
    QImage image;
    int progress = 1;
    for(int idx=0; idx < filenames.count(); idx++)
    {
        // Read image and add to listwidget
        image = QImage(filenames.at(idx));
        if (image.isNull())
            QMessageBox::information(this, "Tiffany", QString("Cannot load %1.").arg(filenames.at(idx)));
        else
        {
            // Cannot paint on indexed8, so convert to grayscale
            if (image.format() == QImage::Format_Indexed8)
                image = image.convertToFormat(QImage::Format_Grayscale8);

            // Remove the next item to be replaced
            if (whom == "replaceAct")
                delete takeItem(rows[0]);

            // Build list item and insert
            QListWidgetItem *newItem = new QListWidgetItem();
            newItem->setToolTip(filenames.at(idx));
            newItem->setData(Qt::UserRole, image);
            newItem->setData(Qt::UserRole+1, 0);
            newItem->setData(Qt::UserRole+2, 0);
            newItem->setIcon(makeIcon(image, false));
            insertItem(rows[0], newItem);

            // Update row for next item
            if (whom == "replaceAct")
            {
                if (rows.count() > 1)
                    rows.remove(0);
                else
                    // Out of replacement items, switch to insert
                    whom = "insertAct";
            }

            // Next item goes after current one
            if ((whom == "openAct") || (whom == "insertAct"))
                rows[0] = rows[0] + 1;

            // TODO
            // if (count() == 1)
            //     self.setCurrentItem(self.item(0))

        }
        // Update progress bar
        emit progressSig("", progress);
        progress = progress + 1;
    }

    // Remove any remaining selections
    if (whom == "replaceAct")
        for(int idx=rows.count()-1;idx >= 0; idx--)
            delete takeItem(rows[idx]);

    // Cleanup status bar
    emit progressSig("", -1);

    // (Re)number all the loaded pages
    for(int idx = 0; idx < count(); idx++)
        item(idx)->setText(QString::number(idx+1));

    // TODO
    // Signal redraw
    // self.currentItemChanged.emit(self.currentItem(), None)
}

void Bookmarks::saveFiles()
{
    qInfo() << QObject::sender()->objectName();
}

void Bookmarks::saveAsFiles()
{
    qInfo() << QObject::sender()->objectName();
}

void Bookmarks::createTIFF()
{
    qInfo() << QObject::sender()->objectName();
}

QIcon Bookmarks::makeIcon(QImage image, bool flag)
{
    // Fill background
    QImage qimg(100, 100, QImage::Format_RGB32);
    qimg.fill(QColor(240, 240, 240));

    // Draw image
    QPainter painter(&qimg);
    QImage scaledImage = image.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (scaledImage.width() > scaledImage.height())
    {
        float m = (100 - scaledImage.height()) / 2.0;
        painter.drawImage(QPoint(0,m), scaledImage);
    } else {
        float m = (100 - scaledImage.width()) / 2.0;
        painter.drawImage(QPoint(m,0), scaledImage);
    }

    // Mark if changed
    if (flag)
    {
        QPainterPath path = QPainterPath();
        path.addRect(QRectF(2,2,10,10));
        painter.fillPath(path, Qt::red);
        painter.drawPath(path);
    }
    painter.end();

    // Convert to icon
    QIcon icon = QIcon(QPixmap::fromImage(qimg));
    return icon;
}


