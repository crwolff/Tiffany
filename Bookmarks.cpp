#include "Bookmarks.h"
#include "Config.h"
#include <QDebug>
#include <QFileDialog>
#include <QImage>
#include <QImageWriter>
#include <QInputDialog>
#include <QMessageBox>
#include <QPainter>

Bookmarks::Bookmarks(QWidget * parent) : QListWidget(parent)
{
}

Bookmarks::~Bookmarks()
{
}

// Capture/Release keyboard
void Bookmarks::enterEvent(QEvent *event)
{
    setFocus();
    event->accept();
}

void Bookmarks::leaveEvent(QEvent *event)
{
    clearFocus();
    event->accept();
}

//
// Monitor selection list and update viewer
//
void Bookmarks::itemSelectionChanged()
{
    QList<QListWidgetItem*> items = selectedItems();

    // Set mode
    if (items.count() > 1)
        Config::multiPage = true;
    else
        Config::multiPage = false;

    // Tell viewer what page to show
    if (items.count() > 0)
        emit changePageSig(items.last());
    else
        emit changePageSig(nullptr);
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
            rows.append(row(item));
        if (rows.count() == 0)
        {
            QMessageBox::information(this, cmd + " Files", "Insertion point must be selected");
            return;
        }
        std::sort(rows.begin(), rows.end());
    }
    else // For open, get last item
        rows.append(count());
    int firstIdx = rows[0];

    // Popup file dialog
    QStringList filenames = QFileDialog::getOpenFileNames(this, cmd + " Files", "", "PNG files (*.png);;TIFF files (*.tif *.tiff);;All files(*)");
    if (filenames.isEmpty())
        return;

    // Add progress to status bar
    emit progressSig("Reading...", filenames.count());

    // Open each file in turn and add to listWidget
    int progress = 1;
    for(int idx=0; idx < filenames.count(); idx++)
    {
        // Read image and add to listwidget
        Page page = Page(filenames.at(idx));
        if (page.m_img.isNull())
            QMessageBox::information(this, "Tiffany", QString("Cannot load %1.").arg(filenames.at(idx)));
        else
        {
            // Cannot paint on indexed8, so convert to better format
            if (page.m_img.format() == QImage::Format_Indexed8)
            {
                if (page.m_img.allGray())
                    page.m_img = page.m_img.convertToFormat(QImage::Format_Grayscale8);
                else
                    page.m_img = page.m_img.convertToFormat(QImage::Format_RGB32);
            }

            // If image has 2 colors, but they aren't black and white, promote to RGB
            if ((page.m_img.format() == QImage::Format_Mono) && (page.m_img.colorCount() == 2))
            {
                if ((page.m_img.color(0) == 0xFFFFFFFF) && (page.m_img.color(1) == 0xFF000000))
                    ;
                else if ((page.m_img.color(0) == 0xFF000000) && (page.m_img.color(1) == 0xFFFFFFFF))
                    ;
                else
                    page.m_img = page.m_img.convertToFormat(QImage::Format_RGB32);
            }

            // Remove the next item to be replaced
            if (cmd == "Replace")
                delete takeItem(rows[0]);

            // Build list item and insert
            QListWidgetItem *newItem = new QListWidgetItem();
            newItem->setToolTip(filenames.at(idx));
            newItem->setData(Qt::UserRole, QVariant::fromValue(page));
            newItem->setIcon(makeIcon(page.m_img, page.modified()));
            QString txt = QFileInfo(filenames.at(idx)).fileName();
            int suffix = txt.lastIndexOf(".");
            if (suffix > 0)
                txt = txt.left(suffix);
            if (txt.length() >= 13)
                txt = txt.left(5) + ".." + txt.right(5);
            newItem->setText(txt);
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

    // Select first item read in
    setCurrentRow(firstIdx);
}

//
// Open files and append to list
//
void Bookmarks::openFiles()
{
    readFiles("Open");
}

//
// Open files and insert into list
//
void Bookmarks::insertFiles()
{
    readFiles("Insert");
}

//
// Open files and replace ones already in list
//
void Bookmarks::replaceFiles()
{
    readFiles("Replace");
}

// Common save routine
bool Bookmarks::saveCommon(QListWidgetItem* itemPtr, QString &fileName, QString &backupName)
{
    // Attempt to create backup
    if (QFileInfo(fileName).exists())
    {
        // If original isn't writeable, flag error
        if (!QFileInfo(fileName).isWritable())
            return false;

        // Delete the backup if it is writable
        if (QFileInfo(backupName).exists() && QFileInfo(backupName).isWritable())
            QFile(backupName).remove();

        // Rename original to backup
        if (QFile(fileName).rename(backupName) == false)
            return false;
    }

    // Save image to <fileName>
    Page page = itemPtr->data(Qt::UserRole).value<Page>();
    QImageWriter writer(fileName);
    //writer.setCompression(100);     // TIF is LZW, no Group 4 option
    if (writer.write(page.m_img) == false)
        return false;

    // Update item
    page.flush();
    itemPtr->setData(Qt::UserRole, QVariant::fromValue(page));
    itemPtr->setIcon(makeIcon(page.m_img, false));

    // No errors
    return true;
}

//
// Save selected files, making backups
//
void Bookmarks::saveFiles()
{
    int writeErr = 0;

    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }

    // Add progress to status bar
    emit progressSig("Saving...", selection.count());

    int progress = 1;
    foreach(QListWidgetItem* itemPtr, selection)
    {
        // Skip if unchanged
        Page page = itemPtr->data(Qt::UserRole).value<Page>();
        if (!page.modified())
            continue;

        // Get the filenames
        QString fileName = itemPtr->toolTip();
        QString backupName = fileName + ".bak";

        // Create the save
        if (!saveCommon(itemPtr, fileName, backupName))
            writeErr++;

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }

    // Cleanup status bar
    emit progressSig("", -1);

    // Report errors
    if (writeErr != 0)
        QMessageBox::information(this, "Tiffany", QString("%1 files couldn't be written").arg(writeErr));
}

//
// Save selected files, making backups
//
void Bookmarks::saveToDir()
{
    int writeErr = 0;

    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }

    // Get directory to save into
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir == "")
        return;

    // Add progress to status bar
    emit progressSig("Saving...", selection.count());

    int progress = 1;
    foreach(QListWidgetItem* itemPtr, selection)
    {
        // Get the filenames
        QString oldName = itemPtr->toolTip();
        QString fileName = dir + "/" + QFileInfo(oldName).fileName();
        itemPtr->setToolTip(fileName);
        QString backupName = fileName + ".bak";

        // Create the save
        if (!saveCommon(itemPtr, fileName, backupName))
            writeErr++;

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }

    // Cleanup status bar
    emit progressSig("", -1);

    // Report errors
    if (writeErr != 0)
        QMessageBox::information(this, "Tiffany", QString("%1 files couldn't be written").arg(writeErr));
}

//
// Check if any pages have been modified and not saved
//
bool Bookmarks::anyModified()
{
    for(int idx=0; idx<count(); idx++)
    {
        Page page = item(idx)->data(Qt::UserRole).value<Page>();
        if (page.modified())
            return true;
    }
    return false;
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
    {
        // Skip if changed
        Page page = item->data(Qt::UserRole).value<Page>();
        if (page.modified())
        {
            QMessageBox::StandardButton resBtn = QMessageBox::question( this, "Tiffany",
                    item->toolTip() + " has been modified, are you sure?\n",
                    QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
            if (resBtn == QMessageBox::Cancel)
                break;
            if (resBtn == QMessageBox::Yes)
                delete item;
        }
        else
            delete item;
    }
    // Since entire selection was deleted, select whatever the listwidget picked as current
    if (currentItem() != nullptr)
        currentItem()->setSelected(true);
}

//
// Erase selected pages and insert 'Blank' into center
//
void Bookmarks::blankPage()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
    {
        QListWidgetItem* last = selection.last();
        selection.clear();
        selection.append(last);
    }

    bool ok;
    QString text = QInputDialog::getText(this, tr("New Page Contents"),
                                         "", QLineEdit::Normal,
                                         "BLANK", &ok);
    if (ok)
    {
        // Add progress to status bar
        emit progressSig("Blanking...", selection.count());

        // Blank each page in turn
        int progress = 1;
        foreach(QListWidgetItem* item, selection)
        {
            Page page = item->data(Qt::UserRole).value<Page>();
            page.push();
            QImage img = page.m_img;

            // Erase and draw text
            QPainter p(&img);
            p.fillRect(img.rect(), Config::bgColor);
            p.setPen(Config::fgColor);
            p.setFont(Config::textFont);
            p.drawText(img.rect(), Qt::AlignCenter, text);
            p.end();

            // Update list with new image
            page.m_img = img;
            item->setData(Qt::UserRole, QVariant::fromValue(page));
            item->setIcon(makeIcon(page.m_img, page.modified()));

            // Update progress
            emit progressSig("", progress);
            progress = progress + 1;
        }
        // Cleanup status bar
        emit progressSig("", -1);

        // Update Viewer
        emit updatePageSig(false);
    }
}

//
// Remove background from multiple pages
//
void Bookmarks::removeBG()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Background...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        QImage mask = page.colorSelect(QColor(Qt::white).rgb(), Config::bgRemoveThreshold);
        page.push();
        page.applyMask(mask, Config::bgColor);

        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Remove speckles
//
void Bookmarks::despeckle()
{
    int blobs;

    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Despeckle...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        QImage mask = page.despeckle(Config::despeckleArea, false, &blobs);
        if (blobs > 0)
        {
            page.push();
            page.applyMask(mask, Config::bgColor);
            item->setData(Qt::UserRole, QVariant::fromValue(page));
            item->setIcon(makeIcon(page.m_img, page.modified()));
        }

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Remove voids
//
void Bookmarks::devoid()
{
    int blobs;

    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Devoid...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        QImage mask = page.despeckle(Config::devoidArea, true, &blobs);
        if (blobs > 0)
        {
            page.push();
            page.applyMask(mask, Config::fgColor);
            item->setData(Qt::UserRole, QVariant::fromValue(page));
            item->setIcon(makeIcon(page.m_img, page.modified()));
        }

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Deskew pages
//
void Bookmarks::deskew()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Deskew...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        float angle = page.calcDeskew();
        QImage img = page.deskew(angle);
        page.push();
        page.applyDeskew(img);

        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Convert to grayscale
//
void Bookmarks::toGrayscale()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Grayscale...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        page.push();
        page.toGrayscale();
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Convert to binary using Otsu's algorithm
//
void Bookmarks::toBinary()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Binary...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();

        // If last operation converted to mono, undo it
        if ((page.m_img.format() == QImage::Format_Mono) && (page.peek().format() != QImage::Format_Mono))
            page.undo();

        page.push();
        page.toBinary(false);
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Convert to binary using adaptive threshold
//
void Bookmarks::toAdaptive()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Adaptive...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();

        // If last operation converted to mono, undo it
        if ((page.m_img.format() == QImage::Format_Mono) && (page.peek().format() != QImage::Format_Mono))
            page.undo();

        page.push();
        page.toBinary(true);
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Convert to binary using diffuse dithering
//
void Bookmarks::toDithered()
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }
    if (!Config::multiPage)
        return;

    // Add progress to status bar
    emit progressSig("Dithering...", selection.count());

    // Blank each page in turn
    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();

        // If last operation converted to mono, undo it
        if ((page.m_img.format() == QImage::Format_Mono) && (page.peek().format() != QImage::Format_Mono))
            page.undo();

        page.push();
        page.toDithered();
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }
    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Rotate selected items
//  1 = 90
//  2 = 180
//  3 = 270
//
void Bookmarks::rotateSelection(int rot)
{
    QTransform tmat = QTransform().rotate(rot * 90.0);

    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }

    // Add progress to status bar
    emit progressSig("Rotating...", selection.count());

    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        page.push();
        QImage img = page.m_img;

        // Rotate image based on transform
        img = img.transformed(tmat, Qt::SmoothTransformation);

        // Update list with new image
        page.m_img = img;
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }

    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(true);
}

//
// Rotate clockwise
//
void Bookmarks::rotateCW()
{
    rotateSelection(1);
}

//
// Rotate counter-clockwise
//
void Bookmarks::rotateCCW()
{
    rotateSelection(3);
}

//
// Rotate clockwise
//
void Bookmarks::rotate180()
{
    rotateSelection(2);
}

//
// Mirror selected items
//  1 = horizontal
//  2 = vertical
//  3 = both
//
void Bookmarks::mirrorSelection(int dir)
{
    // Get list of all selected items
    QList<QListWidgetItem*> selection = selectedItems();
    if (selection.count() == 0)
    {
        QMessageBox::information(this, "Tiffany", "Nothing selected");
        return;
    }

    // Add progress to status bar
    emit progressSig("Rotating...", selection.count());

    int progress = 1;
    foreach(QListWidgetItem* item, selection)
    {
        Page page = item->data(Qt::UserRole).value<Page>();
        page.push();
        QImage img = page.m_img;

        // Mirror image
        img = img.mirrored(((dir & 1) == 1), ((dir & 2) == 2));

        // Update list with new image
        page.m_img = img;
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setIcon(makeIcon(page.m_img, page.modified()));

        // Update progress
        emit progressSig("", progress);
        progress = progress + 1;
    }

    // Cleanup status bar
    emit progressSig("", -1);

    // Update Viewer
    emit updatePageSig(false);
}

//
// Mirror across horizontal axis
//
void Bookmarks::mirrorHoriz()
{
    mirrorSelection(1);
}

//
// Mirror across vertical axis
//
void Bookmarks::mirrorVert()
{
    mirrorSelection(2);
}

//
// Make a new icon after editing in Viewer
//
void Bookmarks::updateIcon()
{
    // Get active item
    QList<QListWidgetItem*> items = selectedItems();
    QListWidgetItem* item = items.last();
    Page page = item->data(Qt::UserRole).value<Page>();
    item->setIcon(makeIcon(page.m_img, page.modified()));
}

//
// Make an icon from the image and add a marker if it has changed
//
QIcon Bookmarks::makeIcon(QImage &image, bool flag)
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

//
// Undo last change
//     TODO Should this apply to current page only or all selected?
//
void Bookmarks::undoEdit()
{
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() == 0)
        return;

    // Get active item
    QListWidgetItem* item = items.last();
    Page page = item->data(Qt::UserRole).value<Page>();

    // Revert last edit
    bool flag = page.undo();
    item->setData(Qt::UserRole, QVariant::fromValue(page));
    item->setIcon(makeIcon(page.m_img, page.modified()));

    // Update Viewer
    emit updatePageSig(flag);
}

//
// Redo last undo
//     TODO Should this apply to current page only or all selected?
//
void Bookmarks::redoEdit()
{
    QList<QListWidgetItem*> items = selectedItems();
    if (items.count() == 0)
        return;

    // Get active item
    QListWidgetItem* item = items.last();
    Page page = item->data(Qt::UserRole).value<Page>();

    // Revert last edit
    bool flag = page.redo();
    item->setData(Qt::UserRole, QVariant::fromValue(page));
    item->setIcon(makeIcon(page.m_img, page.modified()));

    // Update Viewer
    emit updatePageSig(flag);
}

