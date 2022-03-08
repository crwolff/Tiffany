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
        self.flushEdits()
        self.currTransform = None
        self.currInverse = None
        self.rubberBand = None
        self.origin = None
        self.scaleBase = 1.0
        self.scaleFactor = 1.0
        self.drawing = False
        self.brushSize = 1
        self.foregroundColor = QtCore.Qt.black
        self.backgroundColor = QtCore.Qt.white
        self.leftMode = "Pointer"

#
# Event handlers
#
    def mousePressEvent(self, event):
        # if left mouse button is pressed
        if self.rubberBand is None:
            self.rubberBand = QRubberBand(QRubberBand.Rectangle,self)
        if event.button() == QtCore.Qt.LeftButton:
            if self.leftMode == "Pointer":
                self.origin = event.pos()
                self.rubberBand.setGeometry(QtCore.QRect(self.origin, QtCore.QSize()))
                self.rubberBand.show()
                event.accept()
            elif self.leftMode == "Fill":
                self.pushImage()
                self.origin = event.pos()
                self.rubberBand.setGeometry(QtCore.QRect(self.origin, QtCore.QSize()))
                self.rubberBand.show()
                event.accept()
            elif self.leftMode == "Draw" or self.leftMode == "Erase":
                self.pushImage()
                self.drawing = True
                self.origin = event.pos()
                self.setCursor(QtCore.Qt.CrossCursor)
                event.accept()
            else:
                QWidget.mousePressEvent(self, event)
        else:
            QWidget.mousePressEvent(self, event)

    # method for tracking mouse activity
    def mouseMoveEvent(self, event):
        if event.buttons() & QtCore.Qt.LeftButton:
            if self.leftMode == "Pointer" or self.leftMode == "Fill":
                self.rubberBand.setGeometry(QtCore.QRect(self.origin, event.pos()).normalized())
                event.accept()
            elif (self.leftMode == "Draw" or self.leftMode == "Erase") and self.drawing:
                color = self.foregroundColor if self.leftMode == "Draw" else self.backgroundColor)
                self.drawLine(self.origin, event.pos(), color)
                self.origin = event.pos()
                event.accept()
            else:
                QWidget.mouseMoveEvent(self, event)
        else:
            QWidget.mouseMoveEvent(self, event)

    # method for mouse left button release
    def mouseReleaseEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            if self.leftMode == "Pointer":
                event.accept()
            elif self.leftMode == "Fill":
                self.rubberBand.hide()
                self.fillArea(self.rubberBand.geometry())
                event.accept()
            elif self.leftMode == "Draw" or self.leftMode == "Erase":
                self.drawing = False
                self.setCursor(QtCore.Qt.ArrowCursor)
                self.imageChangedSig.emit()
                event.accept()
            else:
                QWidget.mouseReleaseEvent(self, event)
        else:
            QWidget.mouseReleaseEvent(self, event)

    def paintEvent(self, event):
        if self.currImage is not None:
            p = QPainter(self)
            p.setTransform(self.currTransform)
            p.drawImage(self.currImage.rect().topLeft(), self.currImage)

    def imageSelected(self, curr, prev):
        self.flushEdits()
        if curr is not None:
            self.currListItem = curr
            self.currImage = curr.data(QtCore.Qt.UserRole)
            self.fitToWindow()
        else:
            self.currListItem = None
            self.currImage = None

#
# Re-implement for scrollArea sizing
#
    def sizeHint(self):
        if self.currImage is not None:
            return self.currImage.size() * self.scaleFactor * self.scaleBase
        else:
            return super().sizeHint()

#
# Draw functions
#
    def pointerMode(self):
        self.leftMode = "Pointer"

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

    def setTransform(self):
        p = QtGui.QTransform()
        p.scale(self.scaleFactor * self.scaleBase, self.scaleFactor * self.scaleBase)
        self.currTransform = p
        (self.currInverse,_) = self.currTransform.inverted()

    # Draw a line with current color
    def drawLine(self, start, finish, color):
        if self.currImage is None:
            return
        p = QPainter(self.currImage)
        p.setTransform(self.currInverse)
        brush = int(self.brushSize * self.scaleFactor * self.scaleBase)
        if brush == 0:
            brush = 1
        p.setPen(QtGui.QPen(color, brush, QtCore.Qt.SolidLine, QtCore.Qt.RoundCap, QtCore.Qt.RoundJoin))
        p.drawLine(start, finish)
        p.end()

        # Update list widget with new image
        self.currListItem.setData(QtCore.Qt.UserRole, self.currImage)
        self.imageChangedSig.emit()
        self.update()

    # fill rectangle with background color
    def fillArea(self, rect):
        if self.currImage is None:
            return

        # Paint the rectangle
        p = QPainter(self.currImage)
        p.setTransform(self.currInverse)
        p.fillRect(rect, self.backgroundColor)
        p.end()

        # Update list widget with new image
        self.currListItem.setData(QtCore.Qt.UserRole, self.currImage)
        self.imageChangedSig.emit()
        self.update()

#
# Undo/Redo State handlers
#
    # Flush edit State
    def flushEdits(self):
        self.undoState = list()
        self.redoState = list()

    # Save current image before making changes
    def pushImage(self):
        if self.currImage is None:
            return

        # Add current image to beginning of redo State
        self.undoState.insert(0, self.currImage.copy())
        if len(self.undoState) > 5:
            self.undoState = self.undoState[:5]

        # Clear redo State
        self.redoState = list()

    # Roll back one edit
    def undoEdit(self):
        if self.currImage is None:
            return

        if len(self.undoState) > 0:
            # Add current image to beginning of redo State
            self.redoState.insert(0, self.currImage.copy())
            if len(self.redoState) > 5:
                self.redoState = self.redoState[:5]

            # Recover image from undo State
            self.currImage = self.undoState[0]
            self.undoState = self.undoState[1:]

            # Update list widget with new image
            self.currListItem.setData(QtCore.Qt.UserRole, self.currImage)
            self.imageChangedSig.emit()
            self.update()

    # Roll forward one edit
    def redoEdit(self):
        if self.currImage is None:
            return

        if len(self.redoState) > 0:
            # Add current image to beginning of undo State
            self.undoState.insert(0, self.currImage.copy())
            if len(self.undoState) > 5:
                self.undoState = self.undoState[:5]

            # Recover image from redo State
            self.currImage = self.redoState[0]
            self.redoState = self.redoState[1:]

            # Update list widget with new image
            self.currListItem.setData(QtCore.Qt.UserRole, self.currImage)
            self.imageChangedSig.emit()
            self.update()

#
# Zoom functions
#
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
        self.setTransform()
        self.updateGeometry()
        self.adjustScrollBars(1.25)
        self.setVisible(True)
        self.zoomSig.emit()

    def zoomOut(self):
        if self.currImage is None:
            return
        self.setVisible(False)
        self.scaleFactor = self.scaleFactor * 0.8
        self.setTransform()
        self.updateGeometry()
        self.adjustScrollBars(0.8)
        self.setVisible(True)
        self.zoomSig.emit()

    def zoomArea(self):
        if self.currImage is None:
            return
        if self.rubberBand is None or self.rubberBand.isHidden():
            return
        self.rubberBand.hide()

        # Get geometry
        rect = self.rubberBand.geometry()

        # Center of rubberBand in percentage
        centerX = rect.center().x() / self.geometry().width()
        centerY = rect.center().y() / self.geometry().height()

        # Size of rectangle
        rectW = rect.width()
        rectH = rect.height()
        if (rectW == 0) or (rectH == 0):
            return

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
        self.setTransform()
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
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # Scale to larger dimension
        self.scaleFactor = 1.0
        if mode == "H":
            self.scaleBase = view[1] / pix[1]
        else:
            self.scaleBase = view[0] / pix[0]
        self.setTransform()
        # Update scrollArea
        self.updateGeometry()
        self.zoomSig.emit()

    # Fit with vertical scrollbar visible
    def fitWidth(self):
        if self.currImage is None:
            return
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # If height is larger dimension, leave space for vertical scroll bar
        self.scaleFactor = 1.0
        if mode == "H":
            self.scaleBase = (view[0] - scrollBarSize) / pix[0]
        else:
            self.scaleBase = view[0] / pix[0]
        self.setTransform()
        # Update scrollArea
        self.updateGeometry()
        self.zoomSig.emit()

    # Fit with horizontal scrollbar visible
    def fitHeight(self):
        if self.currImage is None:
            return
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # If width is larger dimension, leave space for horizontal scroll bar
        self.scaleFactor = 1.0
        if mode == "H":
            self.scaleBase = view[1] / pix[1]
        else:
            self.scaleBase = (view[1] - scrollBarSize) / pix[1]
        self.setTransform()
        # Update scrollArea
        self.updateGeometry()
        self.zoomSig.emit()

    # Fit with only one scrollbar
    def fillWindow(self):
        if self.currImage is None:
            return
        # Dimensions of viewport
        mode,scrollBarSize,view,pix = self.measureAll()
        # Find smaller ratio
        if mode == "H":
            self.fitWidth()
        else:
            self.fitHeight()
