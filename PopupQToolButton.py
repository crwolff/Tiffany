# -*- coding: utf-8 -*-

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QToolButton

class PopupQToolButton(QToolButton):
    def __init__(self, parent=None):
        QToolButton.__init__(self, parent)
        self.setPopupMode(QToolButton.MenuButtonPopup)
        self.setToolButtonStyle(Qt.ToolButtonTextUnderIcon)
        self.triggered.connect(self.setDefaultAction)
