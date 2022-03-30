from tracemalloc import start
from serial import Serial
import serial.tools.list_ports as ports
import os
import sys
import random
import time, serial
from processors import OrderReceiver, SheetParser
import pandas as pd

def send(sheet_parser, arduinoconn):
    print("packet is " + str(sheet_parser.Packet))
    
    #split packet into bytes
    int_array = []
    packet_use = sheet_parser.Packet

    while packet_use > 0:
        int_array.append(packet_use & 0xff)
        packet_use = packet_use >> 8

    #if packet is <5 bytes, need to pad w 0 bytes
    num_bytes = len(int_array)

    if num_bytes < 5:
        to_pad = 5 - num_bytes
        for i in range(to_pad):
            int_array.append(0)
    
    #byte_array = (int(self.Packet)).to_bytes(5, byteorder = 'big', signed=False)
    print("The bytes are : ", int_array)

    #send packet        
    arduinoconn.write(int_array)
    print("bytes sent")

    #wait for acknowledgment from the drone
    print("waiting for ack from the drone")
    val = check_for_ack(arduinoconn=arduinoconn, timeout=30)
    if val:
        print("Drone Received Packet Succesfully")
    else:
        ###SAVE THIS DATA SOMEWHERE###
        print("Error: Timeout Occured --> No Packet Received")
    return val
    
def check_for_ack(arduinoconn, timeout):
    #Get the serial result from the ground station
    #0 if ack has not been received
    #1 if ack has been received
    #fully wait for a response via the while loop
    hospital_msg = arduinoconn.readline() #read line on serial port 
    serial_val = int.from_bytes(hospital_msg, "big")
    start_time = time.time()
    while serial_val != 12554:
        hospital_msg = arduinoconn.readline() #read line on serial port 
        serial_val = int.from_bytes(hospital_msg, "big")
        print("message is " + str(hospital_msg))
        if (time.time() - start_time > timeout):
            return 0
    return 1
    
def TBSM_ground_start(length=1):
    time_to_check = length * (60*60)
    gen_freq = 5 * 4 # * 60

    start_time = time.time()
    loop_time = time.time()
    
    ds_columns = ["District Hospital", "Rural Station", "Quantity for Item 1", "Quantity for Item 2", 
                  "Quantity for Item 3", "Quantity for Item 4", "Quantity for Item 5", "Quantity for Item 6"]

    sheet_parser = SheetParser()
    
    while ((time.time() - start_time) < time_to_check):
        current_time = time.time()
        if (current_time - loop_time) > gen_freq:
            # arduinoconn = serial.Serial(port='/dev/cu.usbserial-0001', baudrate=115200, timeout=.1)
            print(f"Time Passed: {time.time() - start_time:.2f}")
            loop_time = time.time()
            data_list = []
            for i in range(0, len(ds_columns)):
                if i == 0:
                    rand_val = random.randint(0,1)
                elif i == 1:
                    rand_val = random.randint(0,49)
                else:
                    rand_val = random.randint(0,3)
                data_list.append(rand_val)
            order = pd.DataFrame([data_list],columns=ds_columns)
            print(f"ORDER DATAFRAME: {order}")
            sheet_parser.set_data(order)
            valid_check = sheet_parser.check_valid()
            if valid_check[0]:
                encoding = sheet_parser.encode_to_packet()
                print(f"Encoding Value: {encoding}")
                print("Going to Send the Packet Now")
                arduinoconn = serial.Serial(port='/dev/cu.usbserial-0001', baudrate=115200, timeout=.1)
                send_success = send(sheet_parser=sheet_parser, arduinoconn=arduinoconn)




if __name__ == "__main__":
    random.seed(1275)
    print("Starting Time Before System Maintenance Test for the Ground Station")
    TBSM_ground_start(1)
    print("Time Before System Maintenance Test Complete")