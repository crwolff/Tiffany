# -*- coding: utf-8 -*-

from PyQt5 import QtCore, QtGui
from PyQt5.QtWidgets import QToolButton

class ColorQToolButton(QToolButton):
    def __init__(self, parent=None):
        QToolButton.__init__(self, parent)
        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)
        self.setMouseTracking(True)
        self.mode = "Foreground"

    def setIcon(self, foregroundColor, backgroundColor):
        qpix = QtGui.QPixmap(":/images/assets/fg_color.ico")
        painter = QtGui.QPainter(qpix)
        painter.setBrush(QtCore.Qt.SolidPattern)
        painter.fillRect(1,1,34,34,backgroundColor)
        painter.fillRect(29,29,62,62,foregroundColor)
        painter.end()
        QToolButton.setIcon(self, QtGui.QIcon(qpix))

    # Track mouse to show hotspots
    def mouseMoveEvent(self, event):
        if (event.pos().x() <= 25) and (event.pos().y() <= 15):
            self.setToolTip("Set background color")
            self.mode = "Background"
        elif (event.pos().x() > 25) and (event.pos().y() <= 15):
            self.setToolTip("Swap foreground/background colors")
            self.mode = "Swap"
        elif (event.pos().x() <= 25):
            self.setToolTip("Reset colors to default")
            self.mode = "Reset"
        elif (event.pos().x() > 25):
            self.setToolTip("Set foreground color")
            self.mode = "Foreground"
        QToolButton.mouseMoveEvent(self, event)

    def mouseReleaseEvent(self, event):
        QToolButton.mouseReleaseEvent(self, event)
