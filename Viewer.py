# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import qApp, QApplication, QMainWindow, QMessageBox, QProgressBar, QLabel, QGraphicsView
from PyQt5.QtWidgets import QGraphicsScene, QGraphicsPixmapItem
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter


class Viewer(QGraphicsView):
    progressSig = QtCore.pyqtSignal(str, int)
    zoomSig = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        QGraphicsView.__init__(self, parent)
        #self.setDragMode(QGraphicsView.RubberBandDrag);
        self.currImage = None
        self.lastItem = None
        self.scaleFactor = 1.0
        self.lastPos = None
        self.panning = False

        # Set up graphics viewer
        # TODO: add a logo
        self.scene = QGraphicsScene(self)
        self.setScene(self.scene)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.RightButton:
            self.lastPos = event.pos()
            self.panning = True
            self.setCursor(QtCore.Qt.ClosedHandCursor)
            event.accept()
        else:
            QGraphicsView.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        if self.panning:
            delta = event.pos() - self.lastPos
            self.horizontalScrollBar().setValue(self.horizontalScrollBar().value() - delta.x())
            self.verticalScrollBar().setValue(self.verticalScrollBar().value() - delta.y())
            self.lastPos = event.pos()
            event.accept()
        else:
            QGraphicsView.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        if event.button() == QtCore.Qt.RightButton:
            self.panning = False
            self.setCursor(QtCore.Qt.ArrowCursor)
            event.accept()
        else:
            QGraphicsView.mouseReleaseEvent(self, event)

    def imageSelected(self, curr, prev):
        if curr is not None:
            self.currImage = curr.data(QtCore.Qt.UserRole)
            pix = QPixmap.fromImage(self.currImage)
            if self.lastItem is not None:
                self.scene.removeItem(self.lastItem)
            self.scene.setSceneRect(-10.0, -10.0, pix.size().width() + 20.0, pix.size().height() + 20.0)
            self.lastItem = self.scene.addPixmap(pix)
            self.fitToWindow()
        else:
            if self.lastItem is not None:
                self.scene.removeItem(self.lastItem)
                self.lastItem = None

    def zoomIn(self):
        if self.currImage is None:
            return
        self.scale(1.25, 1.25)
        self.scaleFactor = self.scaleFactor * 1.25
        self.zoomSig.emit()

    def zoomOut(self):
        if self.currImage is None:
            return
        self.scale(0.8, 0.8)
        self.scaleFactor = self.scaleFactor * 0.8
        self.zoomSig.emit()

    def fitToWindow(self):
        if self.currImage is None:
            return
        # Compute reference scale factor
        viewW = self.size().width()
        viewH = self.size().height()
        pixW = self.currImage.size().width() + 20
        pixH = self.currImage.size().height() + 20
        if (viewW * pixH > viewH * pixW):
            scale = 0.995 * viewH / pixH
        else:
            scale = 0.995 * viewW / pixW
        self.resetTransform()
        self.scale(scale, scale)
        self.scaleFactor = 1.0
        self.zoomSig.emit()

    def fitWidth(self):
        if self.currImage is None:
            return
        # Compute reference scale factor
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        viewW = self.size().width() - self.verticalScrollBar().width()
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAsNeeded)
        pixW = self.currImage.size().width() + 20
        scale = 0.995 * viewW / pixW
        self.resetTransform()
        self.scale(scale, scale)
        self.scaleFactor = 1.0
        self.zoomSig.emit()

    def fillWindow(self):
        if self.currImage is None:
            return
        # Compute reference scale factor
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        viewW = self.size().width() - self.verticalScrollBar().width()
        viewH = self.size().height() - self.horizontalScrollBar().height()
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAsNeeded)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAsNeeded)
        pixW = self.currImage.size().width() + 20
        pixH = self.currImage.size().height() + 20
        if (viewW * pixH > viewH * pixW):
            scale = 0.995 * viewW / pixW
        else:
            scale = 0.995 * viewH / pixH
        self.resetTransform()
        self.scale(scale, scale)
        self.scaleFactor = 1.0
        self.zoomSig.emit()
