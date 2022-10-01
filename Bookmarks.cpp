#include "Bookmarks.h"
#include "UndoBuffer.h"
#include "QImage2OCV.h"
#include <QColor>
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QTransform>

Bookmarks::Bookmarks(QWidget * parent) : QListWidget(parent)
{
}

Bookmarks::~Bookmarks()
{
}

//
// Load files into bookmark viewer
//
void Bookmarks::readFiles(QString cmd)
{
    // For replace or insert, get list of items
    QVector<int> rows;
    if ((cmd == "Insert") || (cmd == "Replace"))
    {
        QList<QListWidgetItem*> items = selectedItems();
        foreach(QListWidgetItem* item, items)
            rows.append(item->text().toInt() - 1);
        if (rows.count() == 0)
        {
            QMessageBox::information(this, cmd + " Files", "Insertion point must be selected");
            return;
        }
        std::sort(rows.begin(), rows.end());
    }
    else // For open, get last item
        rows.append(count());

    // Popup file dialog
    QStringList filenames = QFileDialog::getOpenFileNames(this, cmd + " Files", "", "Images (*.png)");
    if (filenames.isEmpty())
        return;

    // Add progress to status bar
    emit progressSig("Reading...", filenames.count());

    // Open each file in turn and add to listWidget
    PageData p;
    UndoBuffer ub;
    int progress = 1;
    for(int idx=0; idx < filenames.count(); idx++)
    {
        // Read image and add to listwidget
        p = PageData(filenames.at(idx));
        if (p.isNull())
            QMessageBox::information(this, "Tiffany", QString("Cannot load %1.").arg(filenames.at(idx)));
        else
        {
            // Cannot paint on indexed8, so convert to RGB
            if (p.format() == QImage::Format_Indexed8)
                p = p.convertToFormat(QImage::Format_RGB32);

            // Remove the next item to be replaced
            if (cmd == "Replace")
                delete takeItem(rows[0]);

            // Build list item and insert
            QListWidgetItem *newItem = new QListWidgetItem();
            newItem->setToolTip(filenames.at(idx));
            newItem->setData(Qt::UserRole, QVariant::fromValue(p));
            ub = UndoBuffer();
            newItem->setData(Qt::UserRole+1, QVariant::fromValue(ub));
            newItem->setIcon(makeIcon(p, p.modified()));
            insertItem(rows[0], newItem);

            // Update row for next item
            if (cmd == "Replace")
            {
                if (rows.count() > 1)
                    rows.remove(0);
                else
                    // Out of replacement items, switch to insert
                    cmd = "Insert";
            }

            // Next item goes after current one
            if ((cmd == "Open") || (cmd == "Insert"))
                rows[0] = rows[0] + 1;
        }

        // Select first item read in
        if (count() == 1)
            setCurrentItem(item(0));

        // Update progress bar
        emit progressSig("", progress);
        progress = progress + 1;
    }

    // Remove any remaining selections
    if (cmd == "Replace")
        for(int idx=rows.count()-1;idx >= 0; idx--)
            delete takeItem(rows[idx]);

    // Cleanup status bar
    emit progressSig("", -1);

    // (Re)number all the loaded pages
    for(int idx = 0; idx < count(); idx++)
        item(idx)->setText(QString::number(idx+1));

    // Signal redraw
    emit currentItemChanged(currentItem(), NULL);
}

//
// Convert selected images from color to grayscale
//
void Bookmarks::toGrayscale()
{
    // Get list of all selected items
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() > 0)
    {
        // Add progress to status bar
        emit progressSig("Converting...", items.count());

        int progress = 1;
        foreach(QListWidgetItem* item, items)
        {
            // Convert to grayscale
            PageData oldImage = item->data(Qt::UserRole).value<PageData>();
            if ((oldImage.format() == QImage::Format_Grayscale8) || (oldImage.format() == QImage::Format_Mono))
                continue;
            PageData newImage = oldImage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
            newImage.copyOtherData(oldImage);
            newImage.setChanges(oldImage.changes() + 1);

            // Push old image into the undo buffer
            UndoBuffer ub = item->data(Qt::UserRole+1).value<UndoBuffer>();
            ub.push(oldImage);
            item->setData(Qt::UserRole+1, QVariant::fromValue(ub));

            // Update item
            item->setData(Qt::UserRole, QVariant::fromValue(newImage));
            item->setIcon(makeIcon(newImage, true));

            // Update progress
            emit progressSig("", progress);
            progress = progress + 1;
        }

        // Cleanup status bar
        emit progressSig("", -1);
    }

    // Signal redraw
    emit currentItemChanged(currentItem(), NULL);
}

//
// Convert selected images from grayscale to mono
//
void Bookmarks::toBinary()
{
    // Get list of all selected items
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() > 0)
    {
        // Add progress to status bar
        emit progressSig("Converting...", items.count());

        int progress = 1;
        foreach(QListWidgetItem* item, items)
        {
            PageData oldImage = item->data(Qt::UserRole).value<PageData>();
            if (oldImage.format() == QImage::Format_Mono)
                continue;

            // Push old image into the undo buffer
            UndoBuffer ub = item->data(Qt::UserRole+1).value<UndoBuffer>();
            ub.push(oldImage);
            item->setData(Qt::UserRole+1, QVariant::fromValue(ub));

            // Convert color images to grayscale first
            if (oldImage.format() != QImage::Format_Grayscale8)
            {
                PageData newImage = oldImage.convertToFormat(QImage::Format_Grayscale8, Qt::ThresholdDither);
                newImage.copyOtherData(oldImage);
                oldImage = newImage;
            }

            // Convert to OpenCV
            cv::Mat mat = QImage2OCV(oldImage);

            // Gausian filter to clean up noise
            if (false)
            {
                cv::Mat tmp;
                cv::GaussianBlur(mat, tmp, cv::Size(5,5), 0);
                mat = tmp;
            }

            // Otsu's local threshold
            if (true)
            {
                cv::Mat tmp;
                cv::threshold(mat, tmp, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                mat = tmp;
            }

            // Adaptive threshold - this hollows out diodes, etc
            if (false)
            {
                cv::Mat tmp;
                cv::adaptiveThreshold(mat, tmp, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
                mat = tmp;
            }

            // Convert back to QImage
            PageData newImage = OCV2QImage(mat);

            // Make it mono
            PageData monoImage = newImage.convertToFormat(QImage::Format_Mono,
                    Qt::MonoOnly|Qt::ThresholdDither|Qt::AvoidDither);

            // Restore metadata
            monoImage.copyOtherData(oldImage);
            monoImage.setChanges(oldImage.changes() + 1);

            // Update item
            item->setData(Qt::UserRole, QVariant::fromValue(monoImage));
            item->setIcon(makeIcon(monoImage, true));

            // Update progress
            emit progressSig("", progress);
            progress = progress + 1;
        }

        // Cleanup status bar
        emit progressSig("", -1);
    }

    // Signal redraw
    emit currentItemChanged(currentItem(), NULL);
}

//
// Save selected files, making backups
//
void Bookmarks::saveFiles()
{
    // Get list of all selected items
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() > 0)
    {
        // Add progress to status bar
        emit progressSig("Saving...", items.count());

        int progress = 1;
        foreach(QListWidgetItem* item, items)
        {
            // Skip if unchanged
            PageData image = item->data(Qt::UserRole).value<PageData>();
            if (!image.modified())
                continue;

            // Get the filenames
            QString fileName = item->toolTip();
            QString backupName = fileName + ".bak";

            // If <fileName>.BAK exists, delete it
            if (QFileInfo(backupName).isFile() && QFileInfo(backupName).isWritable())
                QFile(backupName).remove();

            // Rename <fileName> to <fileName>.BAK
            QFile(fileName).rename(backupName);

            // Save image to <fileName>
            image.save(fileName,"PNG");

            // Clear file change marks
            image.setChanges(0);
            image.setRotation(0);

            // Update item
            item->setData(Qt::UserRole, QVariant::fromValue(image));
            item->setIcon(makeIcon(image, false));

            // Flush undo buffer
            UndoBuffer ub = item->data(Qt::UserRole+1).value<UndoBuffer>();
            ub.flush();
            item->setData(Qt::UserRole+1, QVariant::fromValue(ub));

            // Update progress
            emit progressSig("", progress);
            progress = progress + 1;
        }

        // Cleanup status bar
        emit progressSig("", -1);
    }
}

//
// Save selected files to new directory, making backups if required
//
void Bookmarks::saveToDir()
{
    // Get directory to save into
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir == "")
        return;

    // Get list of all selected items
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() > 0)
    {
        // Add progress to status bar
        emit progressSig("Saving...", items.count());

        int progress = 1;
        foreach(QListWidgetItem* item, items)
        {
            // Skip if unchanged
            PageData image = item->data(Qt::UserRole).value<PageData>();
            if (!image.modified())
                continue;

            // Get the filenames
            QString oldName = item->toolTip();
            QString fileName = dir + "/" + QFileInfo(oldName).fileName();
            QString backupName = fileName + ".bak";

            // If <fileName>.BAK exists, delete it
            if (QFileInfo(backupName).isFile() && QFileInfo(backupName).isWritable())
                QFile(backupName).remove();

            // Rename <fileName> to <fileName>.BAK
            QFile(fileName).rename(backupName);

            // Save image to <fileName>
            image.save(fileName,"PNG");

            // Clear file change marks
            image.setChanges(0);
            image.setRotation(0);

            // Update item
            item->setData(Qt::UserRole, QVariant::fromValue(image));
            item->setIcon(makeIcon(image, false));

            // Flush undo buffer
            UndoBuffer ub = item->data(Qt::UserRole+1).value<UndoBuffer>();
            ub.flush();
            item->setData(Qt::UserRole+1, QVariant::fromValue(ub));

            // Update progress
            emit progressSig("", progress);
            progress = progress + 1;
        }

        // Cleanup status bar
        emit progressSig("", -1);
    }
}

//
// Select all even numbered items
//
void Bookmarks::selectEven()
{
    for(int idx=0; idx < count(); idx++)
        item(idx)->setSelected( (idx&1) == 1 );
}

//
// Select all odd numbered items
//
void Bookmarks::selectOdd()
{
    for(int idx=0; idx < count(); idx++)
        item(idx)->setSelected( (idx&1) == 0 );
}

//
// Delete all selected items
//
void Bookmarks::deleteSelection()
{
    QList<QListWidgetItem*> items = selectedItems();
    foreach(QListWidgetItem* item, items)
        delete item;
}

//
// Rotate selected items
//
void Bookmarks::rotateSelection(qreal rot)
{
    QTransform tmat = QTransform().rotate(rot);

    // Get list of all selected items
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() > 0)
    {
        // Add progress to status bar
        emit progressSig("Rotating...", items.count());

        int progress = 1;
        foreach(QListWidgetItem* item, items)
        {
            // Rotate old image and update rotation flag
            PageData oldImage = item->data(Qt::UserRole).value<PageData>();
            PageData rotImage = oldImage.transformed(tmat, Qt::SmoothTransformation);
            rotImage.copyOtherData(oldImage);
            rotImage.setRotation(oldImage.rotation() + rot);

            // Update item
            item->setData(Qt::UserRole, QVariant::fromValue(rotImage));
            item->setIcon(makeIcon(rotImage, rotImage.modified()));

            // Update progress
            emit progressSig("", progress);
            progress = progress + 1;
        }

        // Cleanup status bar
        emit progressSig("", -1);
    }

    // Signal redraw
    emit currentItemChanged(currentItem(), NULL);
}

//
// Make a new icon after editting in Viewer
//
void Bookmarks::updateIcon()
{
    QListWidgetItem *item = currentItem();
    PageData image = item->data(Qt::UserRole).value<PageData>();
    item->setIcon(makeIcon(image, image.modified()));
}

//
// Make an icon from the image and add a marker if it has changed
//
QIcon Bookmarks::makeIcon(PageData &image, bool flag)
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

    // Display format
    QString txt = "";
    switch (image.format())
    {
        case QImage::Format_RGB888:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
            txt = "RGB";
            break;
        case QImage::Format_Grayscale8:
            txt = "GS";
            break;
        case QImage::Format_Mono:
            txt = "BW";
            break;
        default:
            txt = "???";
            break;
    }
    painter.setPen(Qt::black);
    painter.setFont(QFont("Courier", 8));
    painter.drawText(QRect(0,0,100,100), Qt::AlignRight|Qt::AlignBottom, txt);

    painter.end();

    // Convert to icon
    QIcon icon = QIcon(QPixmap::fromImage(qimg));
    return icon;
}
