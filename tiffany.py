#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import math
import sys
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import qApp, QApplication, QMainWindow, QMessageBox, QProgressBar, QLabel
from PyQt5.QtWidgets import QGraphicsScene, QGraphicsPixmapItem
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter

from mainWin import Ui_MainWindow
from PopupQToolButton import PopupQToolButton
from Bookmarks import Bookmarks
from Viewer import Viewer

class Window(QMainWindow, Ui_MainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setupUi(self)
        self.connectSignalsSlots()

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
        self.fitToWindowAct.setText(_translate("MainWindow", "&Fit to\nWindow"))

    def connectSignalsSlots(self):
        self.openAct.triggered.connect(self.listWidget.readFiles)
        self.insertAct.triggered.connect(self.listWidget.readFiles)
        self.replaceAct.triggered.connect(self.listWidget.readFiles)
        self.saveAct.triggered.connect(self.listWidget.writeFiles)
        self.saveAsAct.triggered.connect(self.listWidget.writeFiles)
        self.createTIFFAct.triggered.connect(self.listWidget.writeFiles)
        self.rotateCWAct.triggered.connect(self.listWidget.rotateSelection)
        self.rotateCCWAct.triggered.connect(self.listWidget.rotateSelection)
        self.rotate180Act.triggered.connect(self.listWidget.rotateSelection)
        self.deleteAct.triggered.connect(self.listWidget.deleteSelection)
        self.exitAct.triggered.connect(self.close)
        self.zoomInAct.triggered.connect(self.graphicsView.zoomIn)
        self.zoomOutAct.triggered.connect(self.graphicsView.zoomOut)
        self.fitToWindowAct.triggered.connect(self.graphicsView.fitToWindow)
        self.fitWidthAct.triggered.connect(self.graphicsView.fitWidth)
        self.fillWindowAct.triggered.connect(self.graphicsView.fillWindow)
        #self.aboutAct.triggered.connect(self.about)
        #self.aboutQtAct.triggered.connect(qApp.aboutQt)

        self.listWidget.progressSig.connect(self.updateProgress)
        self.graphicsView.progressSig.connect(self.updateProgress)
        self.graphicsView.zoomSig.connect(self.updateActions)
        self.listWidget.currentItemChanged.connect(self.graphicsView.imageSelected)

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
        self.zoomMenu.addAction(self.fillWindowAct)
        self.zoomToolButton = PopupQToolButton()
        self.zoomToolButton.setMenu(self.zoomMenu)
        self.zoomToolButton.setDefaultAction(self.fitToWindowAct)
        self.toolBar.addWidget(self.zoomToolButton)

    def updateProgress(self, txt, val):
        if txt != "":
            # Add progress to status bar
            self.statusLabel.setText(txt)
            self.pbar = QProgressBar()
            self.pbar.setRange(0, val)
            self.pbar.setValue(0)
            self.statusbar.addWidget(self.pbar)
        elif val < 0:
            # Clean up status bar
            self.statusbar.removeWidget(self.pbar)
            self.statusLabel.setText("Ready")
        else:
            self.pbar.setValue(val)

    def updateActions(self):
        self.zoomInAct.setEnabled(self.graphicsView.scaleFactor < 10.0)
        self.zoomOutAct.setEnabled(self.graphicsView.scaleFactor > 0.1)

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

