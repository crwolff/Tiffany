# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QFileDialog, QMessageBox, QListWidget, QListWidgetItem
from PyQt5.QtGui import QImage, QPixmap, QTransform, QPainter

class Bookmarks(QListWidget):
    progressSig = QtCore.pyqtSignal(str, int)

    def __init__(self, parent=None):
        QListWidget.__init__(self, parent)

    def readFiles(self):
        # Determine button pressed
        whom = self.sender().objectName()
        if whom == "insertAct":
            txt = 'Insert Files'
        elif whom == "replaceAct":
            txt = 'Replace Files'
        else:
            txt = 'Open Files'

        # Popup file dialog
        options = QFileDialog.Options()
        fileNames,_ = QFileDialog.getOpenFileNames(self, txt, '', 'Images (*.png)', options=options)
        if len(fileNames) == 0:
            return

        # For replace or insert, get list of items
        rows = []
        if (whom == 'insertAct') or (whom == 'replaceAct'):
            for x in self.selectedItems():
                rows.append(int(x.text()) - 1)
            if len(rows) == 0:
                QMessageBox.information(self, txt, "Insertion point must be selected")
                return
            rows.sort()

        # Add progress to status bar
        self.progressSig.emit("Reading...", len(fileNames))

        # For replace, remove old pages
        if (whom == 'replaceAct'):
            for idx in sorted(rows,reverse=True):
                self.takeItem(idx)

        # Open each file in turn and add to listWidget
        progress = 1
        for fname in fileNames:
            image = QImage(fname)
            if image.isNull():
                QMessageBox.information(self, "Tiffany", "Cannot load %s." % fname)
            else:
                # Convert from indexed to grayscale
                if image.format() == QImage.Format_Indexed8:
                    image = image.convertToFormat(QImage.Format_Grayscale8)

                # Create the list item
                item = QListWidgetItem();
                item.setToolTip(fname)
                item.setData(QtCore.Qt.UserRole, image)
                item.setIcon(self.makeIcon(image))

                # Insert list item at appropriate place
                if whom == 'openAct':                   # Append to list
                    self.addItem(item)
                elif whom == 'insertAct':               # Insert all before first selection
                    self.insertItem(rows[0], item)
                    rows[0] = rows[0] + 1
                elif whom == 'replaceAct':              # Insert before selection
                    self.insertItem(rows[0], item)
                    if len(rows) > 1:
                        rows = rows[1:]
                    else:
                        rows[0] = rows[0] + 1

            # Update progress bar
            self.progressSig.emit("", progress)
            progress = progress + 1

        # Cleanup status bar
        self.progressSig.emit("", -1)

        # (Re)number all the loaded pages
        for idx in range(self.count()):
            self.item(idx).setText(str(idx+1))

        # Force re-layout
        self.setSpacing(0)

    def writeFiles(self):
        # Determine button pressed
        whom = self.sender().objectName()
        if whom == "saveAct":
            txt = 'Save Files'
        elif whom == "saveAsAct":
            txt = 'Save As Files'
        else:
            txt = 'Create TIFF'
        print(txt + " not implemented")

    def rotateSelection(self):
        # Determine button pressed
        whom = self.sender().objectName()
        if whom == "rotateCWAct":
            tmat = QTransform().rotate(90.0)
        elif whom == "rotateCCWAct":
            tmat = QTransform().rotate(270.0)
        else:
            tmat = QTransform().rotate(180.0)

        # Make a list of all selected items
        rows = []
        for x in self.selectedItems():
            rows.append(int(x.text()) - 1)

        # Update items in place
        if len(rows) > 0:
            for idx in rows:
                oldImage = self.item(idx).data(QtCore.Qt.UserRole)
                rotImage = oldImage.transformed(tmat, QtCore.Qt.SmoothTransformation)

                # Update item
                self.item(idx).setData(QtCore.Qt.UserRole, rotImage)
                self.item(idx).setIcon(self.makeIcon(rotImage))

        # Signal redraw
        self.currentItemChanged.emit(self.currentItem(), None)

        # Force re-layout
        self.setSpacing(0)

    def deleteSelection(self):
        # Make a list of all selected items
        rows = []
        for x in self.selectedItems():
            rows.append(int(x.text()) - 1)
        rows.sort(reverse=True)

        # Remove from list
        for idx in rows:
            self.takeItem(idx)

        # (Re)number all the loaded pages
        for idx in range(self.count()):
            self.item(idx).setText(str(idx+1))

        # Select something near the deleted items
        if self.count() == 0:
            self.setCurrentItem(None)
        elif rows[-1] < self.count():
            self.setCurrentItem(self.item(rows[-1]))
        else:
            self.setCurrentItem(self.item(self.count()-1))

        # Force re-layout
        self.setSpacing(0)

    def makeIcon(self,image):
        # Fill background
        qimg = QImage(100, 100, QImage.Format_RGB32)
        qimg.fill(QtGui.QColor(240, 240, 240))

        # Draw image
        painter = QPainter(qimg)
        scaledImage = image.scaled(100, 100, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
        if scaledImage.width() > scaledImage.height():
            m = (100 - scaledImage.height()) / 2
            painter.drawImage(QtCore.QPoint(0,m), scaledImage)
        else:
            m = (100 - scaledImage.width()) / 2
            painter.drawImage(QtCore.QPoint(m,0), scaledImage)
        painter.end()

        # Convert to icon
        qpix = QPixmap.fromImage(qimg)
        icon = QtGui.QIcon(qpix)
        return icon
