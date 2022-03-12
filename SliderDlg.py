# -*- coding: utf-8 -*-

from PyQt5 import QtCore
from PyQt5.QtWidgets import QDialog
from Slider import Ui_Slider

class SliderDlg(QDialog):
    changed = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):
        QDialog.__init__(self, parent)
        self.ui = Ui_Slider()
        self.ui.setupUi(self)
        self.ui.horizontalScrollBar.setRange(0, 255)
        self.ui.horizontalScrollBar.setValue(128)
        self.ui.horizontalScrollBar.valueChanged.connect(self.changed)

    def getValue(self):
        return self.ui.horizontalScrollBar.value()
