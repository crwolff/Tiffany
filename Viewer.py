# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QLabel, QMessageBox
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter

class Viewer(QLabel):
    progressSig = QtCore.pyqtSignal(str, int)
    zoomSig = QtCore.pyqtSignal()
    imageChangedSig = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        QLabel.__init__(self, parent)
        self.setBackgroundRole(QPalette.Base)
        self.currImage = None
        self.imageRect = None
        self.scaleFactor = 1.0
        self.foregroundColor = QtCore.Qt.black
        self.backgroundColor = QtCore.Qt.white
        self.brushSize = 1
        self.leftMode = "Zoom"
        self.panning = False

    def imageSelected(self, curr, prev):
        if curr is not None:
            self.currImage = curr.data(QtCore.Qt.UserRole)
            self.setPixmap(QPixmap.fromImage(self.currImage))
            self.adjustSize()
        else:
            self.currImage = None

    def pointerMode(self):
        self.leftMode = "Zoom"

    def pencilMode(self):
        self.leftMode = "Draw"

    def eraserMode(self):
        self.leftMode = "Erase"

    def areaFillMode(self):
        self.leftMode = "Fill"

    def setBrush(self):
        whom = self.sender().objectName()
        if whom == "pix1Act":
            self.brushSize = 1
        elif whom == "pix4Act":
            self.brushSize = 4
        elif whom == "pix8Act":
            self.brushSize = 8
        else:
            self.brushSize = 12

    def zoomIn(self):
        if self.currImage is None:
            return
        self.scaleFactor = self.scaleFactor * 1.25
        self.update()
        self.zoomSig.emit()

    def zoomOut(self):
        if self.currImage is None:
            return
        self.scaleFactor = self.scaleFactor * 0.8
        self.update()
        self.zoomSig.emit()

    def zoomSelect(self):
        if self.currImage is None:
            return

    def fitToWindow(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        self.zoomSig.emit()

    def fitWidth(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        self.zoomSig.emit()

    def fillWindow(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        self.zoomSig.emit()
