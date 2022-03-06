import argparse
import pandas as pd
import numpy as np
import time, os, sys, serial
from datetime import datetime
from serial import Serial
import serial.tools.list_ports as ports

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
        self.export_path = "order_exports"
        self.packet_list = []
    
    def _get_order_name(self):
        order_name = datetime.now().strftime("droneorder_%Y%m%d%H%M%S.xlsx")
        return order_name
    
    def get_packet_from_serial(self):
        sample_packet = [0x02, 0x80, 0x45, 0x18, 0x44]
        self.packet_list.append(sample_packet)
        #FINAL PACKET SHOULD BE 0x0280451844
        
    def export_dataframe(self, dataframe):
        order_name = self._get_order_name()
        dataframe.to_excel("{}\\{}".format(self.export_path, order_name))  

    def decode_order_line(self, order_line):
        #print(len(order_line))
        for i, byter_manz in enumerate(order_line):
            new_shift = (byter_manz) << (8 * (len(order_line) -i - 1))
            if i == 0:
                new_value = new_shift
            else:
                new_value = new_value | new_shift    
        
        order = new_value

        df_list = []
        for i in range(0, 7):
            if i == 0:
                df_list.append(order & 0x3f)
                order = order >> 6
            else:
                df_list.append(order & 0x1f)
                if i != 6: order = order >> 5
        
        return df_list
        

    def format_packet_into_df(self):
        df = None
        self.get_packet_from_serial()
        while len(self.packet_list) > 0:
            order_line = self.packet_list.pop(0)
            df_row = self.decode_order_line(order_line)
            if df is None: 
                df = pd.DataFrame([df_row], columns=["Rural Station", "Item 1", "Item 2", "Item 3", "Item 4", "Item 5", "Item 6"])
            else:
                df.loc[len(df)] = df_row
        return df

if __name__ == "__main__":
    odr = OrderReceiver()
    df = odr.format_packet_into_df()
    odr.export_dataframe(df)

