import argparse
import pandas as pd
import numpy as np
import time, os, sys, serial
from datetime import datetime
from serial import Serial
import serial.tools.list_ports as ports


class SheetParser():
    def __init__(self, sheet_path=None):
        self.DistrictHospital = 0
        self.RuralStation = 0
        self.ItemOne = 0
        self.ItemTwo = 0
        self.ItemThree = 0
        self.ItemFour = 0
        self.ItemFive = 0
        self.ItemSix = 0
        self.Packet = 0

        self.sheet_df = None
        if sheet_path is not None:
            self.import_sheet(sheet_path)

    def import_sheet(self, sheet_path):
        try:
            self.sheet_df = pd.read_excel(sheet_path)
            print(f"Spreadsheet Succesfully Loaded")
            return 1
        except:
            print(f"No Spreadsheet at Location: {sheet_path}")
            return 0

    def display_data(self):
        if self.sheet_df is not None:
            return f"{self.sheet_df}"
        else:
            return "NO SPREADSHEET LOADED"

    def parse_data(self):
        self.DistrictHospital = self.sheet_df["District Hospital"][0]
        self.RuralStation = self.sheet_df["Rural Station"][0]
        self.ItemOne = self.sheet_df["Quantity for Item 1"][0]
        self.ItemTwo = self.sheet_df["Quantity for Item 2"][0]
        self.ItemThree = self.sheet_df["Quantity for Item 3"][0]
        self.ItemFour = self.sheet_df["Quantity for Item 4"][0]
        self.ItemFive = self.sheet_df["Quantity for Item 5"][0]
        self.ItemSix = self.sheet_df["Quantity for Item 6"][0]
    
    def check_valid(self):
        #1st check that DC is valid (0 or 1)
        if not (self.DistrictHospital == 0 or self.DistrictHospital == 1):
            return 0, "Invalid range for Disctrict Hospital. Please pick 0 or 1."

        if (self.RuralStation > 49 or self.RuralStation < 0):
            return 0, "Invalid range for Rural Station. Please pick a value between 0 and 49."

        sum = self.ItemOne + self.ItemTwo + self.ItemThree + self.ItemFour + self.ItemFive + self.ItemSix
        if (sum > 24):
            return 0, "Invalid quantities. Please reselect to have quantity less than or equal to 24."

        return 1, "Data is valid"

    def encode_to_packet(self):
        #build packet according to  "documentation > Packet Encoding.txt"
        self.Packet = self.ItemSix
        
        self.Packet = self.Packet << 5
        self.Packet += self.ItemFive

        self.Packet = self.Packet << 5
        self.Packet += self.ItemFour

        self.Packet = self.Packet << 5
        self.Packet += self.ItemThree

        self.Packet = self.Packet << 5
        self.Packet += self.ItemTwo

        self.Packet = self.Packet << 5
        self.Packet += self.ItemOne

        self.Packet = self.Packet << 6
        self.Packet += self.RuralStation

        self.Packet = self.Packet << 1
        self.Packet += self.DistrictHospital

        return f"Encoded Packet: {self.Packet}"

    def send(self):
        #note: port changes based on wire connected to computer...
        #on mac, run ls /dev/cu.* to find out which ports are connected
        self.arduinoconn = serial.Serial(port='/dev/cu.usbserial-0001', baudrate=115200, timeout=.1)

        #split packet into bytes
        print(int(self.Packet))
        byte_array = (int(self.Packet)).to_bytes(5, byteorder = 'big')
        print("The bytes are : ", byte_array)
        print(byte_array[3])
        #05 00 8A 30 89

        #send packet byte by byte
        for byte in byte_array:
            self.arduinoconn.write(byte)
            time.sleep(0.05)

        self.arduinoconn.close()

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

