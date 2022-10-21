import shiboken2
from PySide2 import QtCore
from PySide2 import QtWidgets
import maya.OpenMayaUI as apiUI

from rpr import RprUsd

def getMayaMainWindow():
    ptr = apiUI.MQtUtil.mainWindow()
    if ptr is not None:
        return shiboken2.wrapInstance(int(ptr), QtWidgets.QWidget)

def open_window() :
    RprUsd.devicesConfiguration.open_window(getMayaMainWindow(), QtCore.Qt.Window, True, True)
