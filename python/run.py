import argparse
import pandas as pd
import numpy as np
import time, os, sys, serial
from serial import Serial
import serial.tools.list_ports as ports
from PyQt5.QtWidgets import QApplication

from processors import OrderReceiver, SheetParser
from gui import Window

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyleSheet('''
        QWidget{
            font-size:24px;
        }
    ''')

    window = Window()
    window.show()

    sys.exit(app.exec_())

    '''
    sheet_path= "Order.xlsx"

    #Going to turn into an argument
    sp = SheetParser(sheet_path)

    sp.display_data()
    sp.parse_data()
    sp.check_valid()
    sp.encode_to_packet()
    #encrypt
    sp.send()
    '''