# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QWidget, QRubberBand, QMessageBox
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter

class Viewer(QWidget):
    progressSig = QtCore.pyqtSignal(str, int)
    zoomSig = QtCore.pyqtSignal()
    imageChangedSig = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.scrollArea = None  # Set by main window
        self.currListItem = None
        self.currImage = None
        self.currTransform = None
        self.rubberBand = None
        self.origin = None
        self.scaleBase = 1.0
        self.scaleFactor = 1.0
        self.foregroundColor = QtCore.Qt.black
        self.backgroundColor = QtCore.Qt.white
        self.brushSize = 1
        self.leftMode = "Zoom"

    def mousePressEvent(self, event):
        # if left mouse button is pressed
        if self.rubberBand is None:
            self.rubberBand = QRubberBand(QRubberBand.Rectangle,self)
        if event.button() == QtCore.Qt.LeftButton:
            if self.leftMode == "Zoom" or self.leftMode == "Fill":
                self.origin = event.pos()
                self.rubberBand.setGeometry(QtCore.QRect(self.origin, QtCore.QSize()))
                self.rubberBand.show()

    # method for tracking mouse activity
    def mouseMoveEvent(self, event):
        if event.buttons() & QtCore.Qt.LeftButton:
            if self.leftMode == "Zoom" or self.leftMode == "Fill":
                self.rubberBand.setGeometry(QtCore.QRect(self.origin, event.pos()).normalized())

    # method for mouse left button release
    def mouseReleaseEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            if self.leftMode == "Zoom":
                self.rubberBand.hide()
                self.zoomArea(self.rubberBand.geometry())
            elif self.leftMode == "Fill":
                self.rubberBand.hide()
                self.fillArea(self.rubberBand.geometry())

    def paintEvent(self, event):
        if self.currImage is not None:
            p = QPainter(self)
            p.scale(self.scaleFactor * self.scaleBase, self.scaleFactor * self.scaleBase)
            self.currTransform = p.transform()
            p.drawImage(self.currImage.rect().topLeft(), self.currImage)

    def imageSelected(self, curr, prev):
        if curr is not None:
            self.currListItem = curr
            self.currImage = curr.data(QtCore.Qt.UserRole)
            self.fitToWindow()
        else:
            self.currListItem = None
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

    def updateScrollBars(self):
        # Cheesy way to force qscrollArea to recalculate scrollbars
        self.scrollArea.setWidgetResizable(True)

    def adjustScrollBars(self, factor):
        scrollBar = self.scrollArea.horizontalScrollBar
        scrollBar().setValue(int(factor * scrollBar().value() + ((factor - 1) * scrollBar().pageStep() / 2)))
        scrollBar = self.scrollArea.verticalScrollBar
        scrollBar().setValue(int(factor * scrollBar().value() + ((factor - 1) * scrollBar().pageStep() / 2)))

    def zoomIn(self):
        if self.currImage is None:
            return
        self.setVisible(False)
        self.scaleFactor = self.scaleFactor * 1.25
        self.updateGeometry()
        self.adjustScrollBars(1.25)
        self.setVisible(True)
        self.zoomSig.emit()

    def zoomOut(self):
        if self.currImage is None:
            return
        self.setVisible(False)
        self.scaleFactor = self.scaleFactor * 0.8
        self.updateGeometry()
        self.adjustScrollBars(0.8)
        self.setVisible(True)
        self.zoomSig.emit()

    def zoomArea(self, rect):
        if self.currImage is None:
            return

        # Center of rubberBand in percentage
        centerX = rect.center().x() / self.geometry().width()
        centerY = rect.center().y() / self.geometry().height()

        # Size of rectangle
        rectW = rect.width()
        rectH = rect.height()

        # Size of viewport with scrollbars
        scrollBarExtent = self.style().pixelMetric(QtWidgets.QStyle.PM_ScrollBarExtent)
        if self.scrollArea.verticalScrollBar().isVisible():
            viewW = self.parentWidget().width()
        else:
            viewW = self.parentWidget().width() - scrollBarExtent
        if self.scrollArea.horizontalScrollBar().isVisible():
            viewH = self.parentWidget().height()
        else:
            viewH = self.parentWidget().height() - scrollBarExtent

        # Compute larger dimension and scale to it
        if (rectW * viewH > rectH * viewW):
            scale = viewW / rectW
        else:
            scale = viewH / rectH
        self.setVisible(False)
        self.scaleFactor = self.scaleFactor * scale
        self.updateGeometry()
        self.updateScrollBars()

        # Adjust scrollbars so center of rect is center of viewport
        scrollBar = self.scrollArea.horizontalScrollBar
        scrollBar().setValue(int(centerX * scrollBar().maximum() + ((centerX - 1/2) * scrollBar().pageStep())))
        scrollBar = self.scrollArea.verticalScrollBar
        scrollBar().setValue(int(centerY * scrollBar().maximum() + ((centerY - 1/2) * scrollBar().pageStep())))

        self.setVisible(True)
        self.zoomSig.emit()

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
            return("H",scrollBarExtent,(viewW,viewH),(pixW,pixH))
        else:
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

    # fill rectangle with background color
    def fillArea(self, rect):
        if self.currImage is None:
            return

        # Paint the rectangle
        p = QPainter(self.currImage)
        #p.scale(1/(self.scaleFactor * self.scaleBase), 1/(self.scaleFactor * self.scaleBase))
        (transform,_) = self.currTransform.inverted()
        p.setTransform(transform)
        p.fillRect(rect, self.backgroundColor)
        p.end()

        # Update list widget with new image
        self.currListItem.setData(QtCore.Qt.UserRole, self.currImage)
        self.imageChangedSig.emit()
        self.update()
