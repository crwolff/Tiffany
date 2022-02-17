#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import math
import sys
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import qApp, QApplication, QMainWindow, QMessageBox, QFileDialog, QListWidgetItem, QProgressBar, QLabel
from PyQt5.QtWidgets import QGraphicsScene, QGraphicsPixmapItem
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter

from mainWin import Ui_MainWindow

class Window(QMainWindow, Ui_MainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setupUi(self)
        self.connectSignalsSlots()
        self.fileNameList = []
        self.imageList = []
        self.currImage = None
        self.scaleFactor = 1.0

        # Status bar
        self.statusLabel = QLabel("Ready")
        self.statusbar.addWidget(self.statusLabel)

        # Better fit
        _translate = QtCore.QCoreApplication.translate
        self.zoomInAct.setText(_translate("MainWindow", "Zoom\n&In (25%)"))
        self.zoomOutAct.setText(_translate("MainWindow", "Zoom\n&Out (25%)"))
        self.normalSizeAct.setText(_translate("MainWindow", "&Normal\nSize"))

    def connectSignalsSlots(self):
        self.openAct.triggered.connect(self.open)
        self.exitAct.triggered.connect(self.close)
        self.zoomInAct.triggered.connect(self.zoomIn)
        self.zoomOutAct.triggered.connect(self.zoomOut)
        self.normalSizeAct.triggered.connect(self.normalSize)
        self.aboutAct.triggered.connect(self.about)
        self.aboutQtAct.triggered.connect(qApp.aboutQt)

        self.listWidget.currentItemChanged.connect(self.imageSelected)

    def open(self):
        options = QFileDialog.Options()
        fileNames,_ = QFileDialog.getOpenFileNames(self, 'Open Files', '', 'Images (*.png)', options=options)
        if len(fileNames) == 0:
            return

        # Add progress to status bar
        self.statusLabel.setText("Reading...")
        pbar = QProgressBar()
        pbar.setRange(0, len(fileNames))
        pbar.setValue(0)
        self.statusbar.addWidget(pbar)
        for x in fileNames:
            image = QImage(x)
            if image.isNull():
                QMessageBox.information(self, "Tiffany", "Cannot load %s." % x)
            else:
                if image.format() == QImage.Format_Indexed8:
                    image = image.convertToFormat(QImage.Format_Grayscale8)
                self.fileNameList.append(x)
                self.imageList.append(image)

                item = QListWidgetItem();
                item.setText(str(len(self.imageList)))
                qimg = image.scaled(100, 100, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
                qpix = QPixmap.fromImage(qimg)
                icon = QtGui.QIcon(qpix)
                item.setIcon(icon)
                self.listWidget.addItem(item)

                if self.listWidget.count() == 1:
                    itm = self.listWidget.findItems("1", QtCore.Qt.MatchExactly)
                    self.listWidget.setCurrentItem(itm[0])
            pbar.setValue(pbar.value() + 1)
        self.statusbar.removeWidget(pbar)
        self.statusLabel.setText("Ready")

    def imageSelected(self, curr, prev):
        self.currImage = self.imageList[int(curr.text())-1]
        self.normalSize()

    def zoomIn(self):
        self.scaleImage(1.25)

    def zoomOut(self):
        self.scaleImage(0.8)

    def normalSize(self):
        self.scaleFactor = 1.0
        self.scaleImage(1.0)

    def scaleImage(self, factor):
        self.scaleFactor *= factor
        viewSize = self.graphicsView.size() * self.scaleFactor - QtCore.QSize(1,1)
        qimg = self.currImage.scaled(viewSize.width()-5, viewSize.height()-5, QtCore.Qt.KeepAspectRatio, QtCore.Qt.SmoothTransformation)
        pix = QPixmap.fromImage(qimg)
        scn = QGraphicsScene(self)
        scn.addPixmap(pix)
        self.graphicsView.setScene(scn)

        self.zoomInAct.setEnabled(self.scaleFactor < 5.0)
        self.zoomOutAct.setEnabled(self.scaleFactor > 0.2)
        self.normalSizeAct.setEnabled(True)

    def about(self):
        QMessageBox.about(
            self,
            "About Tiffany",
            "<p><b>Tiffany</b> is an image editor tuned for cleaning"
            "up scanned images</p>"
        )

if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = Window()
    win.show()
    sys.exit(app.exec())

