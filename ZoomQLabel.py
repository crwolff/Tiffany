# -*- coding: utf-8 -*-

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QLabel
from PyQt5.QtGui import QImage, QPixmap

class ZoomQLabel(QLabel):
    def __init__(self, parent=None):
        QLabel.__init__(self, parent)
        self.image = None
        self.scaleFactor = 1.0

    def setImage(self,image):
        QLabel.clear(self)

        # Save full resolution image
        self.image = image

        # Blank display
        if image is None:
            return

        # Compute scale factor to fit into window
        pixW = self.image.size().width()
        pixH = self.image.size().height()
        labW = self.size().width()
        labH = self.size().height()
        if (pixW <= 0) or (pixH <= 0) or (labW <= 0) or (labH <= 0):
            return
        if (labW * pixH > labH * pixW):
            qimg = image.scaledToHeight(labH, Qt.SmoothTransformation)
        else:
            qimg = image.scaledToWidth(labW, Qt.SmoothTransformation)
        QLabel.setPixmap(self, QPixmap.fromImage(qimg))

    def resizeEvent(self, event):
        QLabel.resizeEvent(self, event)
        self.setImage(self.image)

    def zoomIn(self):
        return

    def zoomOut(self):
        return
