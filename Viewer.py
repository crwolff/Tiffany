# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import qApp, QApplication, QMainWindow, QMessageBox, QProgressBar, QLabel, QGraphicsView
from PyQt5.QtWidgets import QGraphicsScene, QGraphicsPixmapItem
from PyQt5.QtGui import QImage, QPixmap, QPalette, QPainter


class Viewer(QGraphicsView):
    def __init__(self, parent=None):
        QGraphicsView.__init__(self, parent)
        self.currImage = None
        self.lastItem = None
        self.scaleFactor = 1.0

        # Set up graphics viewer
        # TODO: add a logo
        self.scene = QGraphicsScene(self)
        self.setScene(self.scene)

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

    def zoomOut(self):
        if self.currImage is None:
            return
        self.scale(0.8, 0.8)
        self.scaleFactor = self.scaleFactor * 0.8

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

