# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QWidget, QMessageBox
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter

class Viewer(QWidget):
    progressSig = QtCore.pyqtSignal(str, int)
    zoomSig = QtCore.pyqtSignal()
    imageChangedSig = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.currImage = None
        self.scaleFactor = 1.0
        self.foregroundColor = QtCore.Qt.black
        self.backgroundColor = QtCore.Qt.white
        self.brushSize = 1
        self.leftMode = "Zoom"
        self.panning = False

    def paintEvent(self, event):
        if self.currImage is not None:
            p = QPainter(self)
            p.scale(self.scaleFactor, self.scaleFactor)
            p.drawImage(self.currImage.rect().topLeft(), self.currImage)

    def imageSelected(self, curr, prev):
        if curr is not None:
            self.currImage = curr.data(QtCore.Qt.UserRole)
            self.updateGeometry()
        else:
            self.currImage = None
    
    def sizeHint(self):
        if self.currImage is not None:
            return self.currImage.size()
        else:
            return super().sizeHint()

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

    def zoomOut(self):
        if self.currImage is None:
            return

    def zoomSelect(self):
        if self.currImage is None:
            return

    def fitToWindow(self):
        if self.currImage is None:
            return

    def fitWidth(self):
        if self.currImage is None:
            return

    def fillWindow(self):
        if self.currImage is None:
            return
