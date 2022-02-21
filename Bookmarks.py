# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QFileDialog, QMessageBox, QListWidget, QListWidgetItem
from PyQt5.QtGui import QImage, QPixmap

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
            rows.sort()
            if len(rows) == 0:
                QMessageBox.information(self, txt, "Insertion point must be selected")
                return

        # Add progress to status bar
        self.progressSig.emit("Reading...", len(fileNames))

        # Open each file in turn and add to listWidget
        progress = 1
        for x in fileNames:
            image = QImage(x)
            if image.isNull():
                QMessageBox.information(self, "Tiffany", "Cannot load %s." % x)
            else:
                # Convert from indexed to grayscale
                if image.format() == QImage.Format_Indexed8:
                    image = image.convertToFormat(QImage.Format_Grayscale8)

                # Make icon
                qimg = image.scaled(100, 100, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
                qpix = QPixmap.fromImage(qimg)
                icon = QtGui.QIcon(qpix)

                # Build list item
                item = QListWidgetItem();
                item.setToolTip(x)
                item.setData(QtCore.Qt.UserRole, image)
                item.setIcon(icon)

                # Insert list item at appropriate place
                if whom == 'openAct':                   # Append to list
                    self.addItem(item)
                elif whom == 'insertAct':               # Insert before selection
                    self.insertItem(rows[0], item)
                    rows[0] = rows[0] + 1
                elif whom == 'replaceAct':              # Replace selection, then treat as insert
                    self.takeItem(rows[0])
                    self.insertItem(rows[0], item)
                    if len(rows) > 1:
                        rows = rows[1:]
                    else:
                        whom = 'insertAct'
                        rows[0] = rows[0] + 1

            # Update progress bar
            self.progressSig.emit("", progress)
            progress = progress + 1

        # Cleanup status bar
        self.progressSig.emit("", -1)

        # (Re)number all the loaded pages
        for x in range(self.count()):
            self.item(x).setText(str(x+1))

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

