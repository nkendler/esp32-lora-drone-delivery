import argparse
import pandas as pd
import numpy as np
import time, os, sys, serial
from serial import Serial
import serial.tools.list_ports as ports

from order_receiver import OrderReceiver

class OrderReceiver():
    """
    GENERAL FLOW

    *** ASSUMING COMPUTER IS ALWAYS CONNECTED TO THE DEVICE HENCE FULL TIME SERIAL ACCESS***

    1. Program starts and begins listening for incoming orders
    2. Orders will come via serial communication from the hostipal device
    3. As an order comes in:
        - Update the console to show an order received
        - Show either the full order or summary
        - Save the order in an output folder, formatted the way we want it to be
    
    """
    def __init__(self): 
        print("Initializing Order Receiver")
        self.packet_list = []
    
    def get_packet_from_serial(self):
        pass

    def decode_order_line(self, order_line):
        pass

    def format_packet_into_df(self):
        while len(self.packet_list) > 0:
            order_line = self.packet_list.pop(0)
            df_row = self.decode_order_line(order_line)
            


    def 