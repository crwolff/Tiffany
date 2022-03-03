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
        self.scaleBase = 1.0
        self.scaleFactor = 1.0
        self.foregroundColor = QtCore.Qt.black
        self.backgroundColor = QtCore.Qt.white
        self.brushSize = 1
        self.leftMode = "Zoom"
        self.panning = False
        self.scrollArea = None

    def paintEvent(self, event):
        if self.currImage is not None:
            p = QPainter(self)
            p.scale(self.scaleFactor * self.scaleBase, self.scaleFactor * self.scaleBase)
            p.drawImage(self.currImage.rect().topLeft(), self.currImage)

    def imageSelected(self, curr, prev):
        if curr is not None:
            self.currImage = curr.data(QtCore.Qt.UserRole)
            self.fitToWindow()
        else:
            self.currImage = None
    
    def sizeHint(self):
        if self.currImage is not None:
            return self.currImage.size() * self.scaleFactor * self.scaleBase
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
        self.scaleFactor = self.scaleFactor * 1.25
        self.setVisible(False)
        self.adjustScrollBars(1.25)
        self.updateGeometry()
        self.setVisible(True)
        self.zoomSig.emit()

    def zoomOut(self):
        if self.currImage is None:
            return
        self.scaleFactor = self.scaleFactor * 0.8
        self.setVisible(False)
        self.adjustScrollBars(0.8)
        self.updateGeometry()
        self.setVisible(True)
        self.zoomSig.emit()

    def zoomSelect(self):
        if self.currImage is None:
            return

    def adjustScrollBars(self, factor):
        scrollBar = self.scrollArea.horizontalScrollBar
        scrollBar().setValue(int(factor * scrollBar().value() + ((factor - 1) * scrollBar().pageStep() / 2)))
        scrollBar = self.scrollArea.verticalScrollBar
        scrollBar().setValue(int(factor * scrollBar().value() + ((factor - 1) * scrollBar().pageStep() / 2)))

    def measureAll(self):
        # Size of viewport without scrollbars
        scrollBarExtent = self.style().pixelMetric(QtWidgets.QStyle.PM_ScrollBarExtent)
        if self.scrollArea.verticalScrollBar().isVisible():
            viewW = self.parentWidget().width() + scrollBarExtent
        else:
            viewW = self.parentWidget().width()
        if self.scrollArea.horizontalScrollBar().isVisible():
            viewH = self.parentWidget().height() + scrollBarExtent
        else:
            viewH = self.parentWidget().height()
        # Size of image
        pixW = self.currImage.size().width()
        pixH = self.currImage.size().height()
        # Compute larger dimension and scale to it
        if (viewW * pixH > viewH * pixW):
            #print("H",scrollBarExtent,(viewW,viewH),(pixW,pixH))
            return("H",scrollBarExtent,(viewW,viewH),(pixW,pixH))
        else:
            #print("W",scrollBarExtent,(viewW,viewH),(pixW,pixH))
            return("W",scrollBarExtent,(viewW,viewH),(pixW,pixH))

    # Fit with no scroll bars
    def fitToWindow(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # Scale to larger dimension
        if mode == "H":
            self.scaleBase = view[1] / pix[1]
        else:
            self.scaleBase = view[0] / pix[0]
        # Update scrollArea
        self.updateGeometry()
        self.zoomSig.emit()

    # Fit with vertical scrollbar visible
    def fitWidth(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # If height is larger dimension, leave space for vertical scroll bar
        if mode == "H":
            self.scaleBase = (view[0] - scrollBarSize) / pix[0]
        else:
            self.scaleBase = view[0] / pix[0]
        # Update scrollArea
        self.updateGeometry()
        self.zoomSig.emit()

    # Fit with horizontal scrollbar visible
    def fitHeight(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # If width is larger dimension, leave space for horizontal scroll bar
        if mode == "H":
            self.scaleBase = view[1] / pix[1]
        else:
            self.scaleBase = (view[1] - scrollBarSize) / pix[1]
        # Update scrollArea
        self.updateGeometry()
        self.zoomSig.emit()

    # Fit with only one scrollbar
    def fillWindow(self):
        if self.currImage is None:
            return
        self.scaleFactor = 1.0
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # Find smaller ratio
        if mode == "H":
            self.fitWidth()
        else:
            self.fitHeight()
