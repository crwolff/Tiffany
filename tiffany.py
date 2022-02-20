#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import math
import sys
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import qApp, QApplication, QMainWindow, QMessageBox, QFileDialog, QListWidgetItem, QProgressBar, QLabel
from PyQt5.QtWidgets import QGraphicsScene, QGraphicsPixmapItem
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter

from mainWin import Ui_MainWindow
from PopupQToolButton import PopupQToolButton
from Bookmarks import Bookmarks

class Window(QMainWindow, Ui_MainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setupUi(self)
        self.connectSignalsSlots()
        self.currImage = None
        self.lastItem = None
        self.scaleFactor = 1.0

        # Set up graphics viewer
        # TODO: add a logo
        self.scene = QGraphicsScene(self)
        self.graphicsView.setScene(self.scene)

        # Status bar
        self.statusLabel = QLabel("Ready")
        self.statusbar.addWidget(self.statusLabel)

        # Hand assemble toolbar items
        self.buildToolBar()

        # Better fit
        _translate = QtCore.QCoreApplication.translate
        self.rotateCWAct.setText(_translate("MainWindow", "Rotate\nCW"))
        self.rotateCCWAct.setText(_translate("MainWindow", "Rotate\nCCW"))
        self.rotate180Act.setText(_translate("MainWindow", "Rotate\n180"))
        self.zoomInAct.setText(_translate("MainWindow", "Zoom\n&In (25%)"))
        self.zoomOutAct.setText(_translate("MainWindow", "Zoom\n&Out (25%)"))

    def connectSignalsSlots(self):
        self.openAct.triggered.connect(self.listWidget.readFiles)
        self.insertAct.triggered.connect(self.listWidget.readFiles)
        self.replaceAct.triggered.connect(self.listWidget.readFiles)
        self.saveAct.triggered.connect(self.listWidget.writeFiles)
        self.saveAsAct.triggered.connect(self.listWidget.writeFiles)
        self.createTIFFAct.triggered.connect(self.listWidget.writeFiles)
        self.exitAct.triggered.connect(self.close)
        self.zoomInAct.triggered.connect(self.zoomIn)
        self.zoomOutAct.triggered.connect(self.zoomOut)
        self.fitToWindowAct.triggered.connect(self.fitToWindow)
        #self.aboutAct.triggered.connect(self.about)
        #self.aboutQtAct.triggered.connect(qApp.aboutQt)

        self.listWidget.currentItemChanged.connect(self.imageSelected)

    def buildToolBar(self):
        self.openMenu = QtWidgets.QMenu(self)
        self.openMenu.addAction(self.openAct)
        self.openMenu.addAction(self.insertAct)
        self.openMenu.addAction(self.replaceAct)
        self.openToolButton = PopupQToolButton()
        self.openToolButton.setMenu(self.openMenu)
        self.openToolButton.setDefaultAction(self.openAct)
        self.toolBar.addWidget(self.openToolButton)

        self.saveMenu = QtWidgets.QMenu(self)
        self.saveMenu.addAction(self.saveAct)
        self.saveMenu.addAction(self.saveAsAct)
        self.saveMenu.addAction(self.createTIFFAct)
        self.saveToolButton = PopupQToolButton()
        self.saveToolButton.setMenu(self.saveMenu)
        self.saveToolButton.setDefaultAction(self.saveAct)
        self.toolBar.addWidget(self.saveToolButton)

        self.rotateMenu = QtWidgets.QMenu(self)
        self.rotateMenu.addAction(self.rotateCWAct)
        self.rotateMenu.addAction(self.rotateCCWAct)
        self.rotateMenu.addAction(self.rotate180Act)
        self.rotateToolButton = PopupQToolButton()
        self.rotateToolButton.setMenu(self.rotateMenu)
        self.rotateToolButton.setDefaultAction(self.rotateCWAct)
        self.toolBar.addWidget(self.rotateToolButton)

        self.toolBar.addAction(self.deleteAct)
        self.toolBar.addAction(self.zoomOutAct)
        self.toolBar.addAction(self.zoomInAct)

        self.zoomMenu = QtWidgets.QMenu(self)
        self.zoomMenu.addAction(self.fitToWindowAct)
        self.zoomMenu.addAction(self.fitWidthAct)
        self.zoomMenu.addAction(self.fit100Act)
        self.zoomToolButton = PopupQToolButton()
        self.zoomToolButton.setMenu(self.zoomMenu)
        self.zoomToolButton.setDefaultAction(self.fitToWindowAct)
        self.toolBar.addWidget(self.zoomToolButton)

    def imageSelected(self, curr, prev):
        self.currImage = curr.data(QtCore.Qt.UserRole)
        pix = QPixmap.fromImage(self.currImage)
        if self.lastItem is not None:
            self.scene.removeItem(self.lastItem)
        self.scene.setSceneRect(-10.0, -10.0, pix.size().width() + 20.0, pix.size().height() + 20.0)
        self.lastItem = self.scene.addPixmap(pix)
        self.fitToWindow()

    def zoomIn(self):
        if self.currImage is None:
            return
        self.graphicsView.scale(1.25, 1.25)
        self.scaleFactor = self.scaleFactor * 1.25
        self.zoomInAct.setEnabled(self.scaleFactor < 5.0)

    def zoomOut(self):
        if self.currImage is None:
            return
        self.graphicsView.scale(0.8, 0.8)
        self.scaleFactor = self.scaleFactor * 0.8
        self.zoomOutAct.setEnabled(self.scaleFactor > 0.2)

    def fitToWindow(self):
        if self.currImage is None:
            return
        # Compute reference scale factor
        viewW = self.graphicsView.size().width()
        viewH = self.graphicsView.size().height()
        pixW = self.currImage.size().width() + 20
        pixH = self.currImage.size().height() + 20
        if (viewW * pixH > viewH * pixW):
            scale = 0.995 * viewH / pixH
        else:
            scale = 0.995 * viewW / pixW
        self.graphicsView.resetTransform()
        self.graphicsView.scale(scale, scale)

        self.scaleFactor = 1.0
        self.zoomInAct.setEnabled(True)
        self.zoomOutAct.setEnabled(True)
        self.fitToWindowAct.setEnabled(True)

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

