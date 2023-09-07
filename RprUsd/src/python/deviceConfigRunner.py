#
# Copyright 2023 Advanced Micro Devices, Inc
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#    http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


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
