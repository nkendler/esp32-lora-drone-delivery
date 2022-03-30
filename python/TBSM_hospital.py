from turtle import update
from serial import Serial
import serial.tools.list_ports as ports
import os
import sys
import time, serial, csv
from processors import OrderReceiver, SheetParser
import random

def TBSM_hospital_start(length=1):
    time_to_check = length * (60*60)
    update_freq = 5

    #Setup loop to continually read until time_to_check has passed
    #If an order comes in, process it and compare values to re-generated values
        #Save the time at which the order came in, and whether it was fully correct

    result_list = []
    start_time = time.time()
    loop_time = time.time()
    order_rec = OrderReceiver()
    arduinoconn = serial.Serial(port='/dev/cu.usbserial-0001', baudrate=115200, timeout=.1)

    while ((time.time() - start_time) < time_to_check):
        current_time = time.time()
        #Check if any packets have been received
        df = order_rec.format_packet_into_df(arduinoconn) 
        receive_list = []
        if df is not None:
            print(f"\nPacket Recieved")
            #Accumulate list of received values
            for column in df.columns:
                receive_list.append(int(df[column][0]))

            #Generate the values that would have been sent
            data_list = []
            for i in range(0, len(df.columns) + 1):
                if i == 0:
                    rand_val = random.randint(0,1)
                elif i == 1:
                    rand_val = random.randint(0,49)
                else:
                    rand_val = random.randint(0,3)
                data_list.append(rand_val)
            comp_list = data_list[1:]
            
            #Compare for correctness
            same_val = True
            for i in range(0, len(comp_list)):
                same_val = comp_list[i] == receive_list[i]

            result_list.append((current_time-start_time, same_val))
            
            if same_val:
                print(f"{current_time - start_time}: Values Received == Values Generated")
            else:
                print(f"{current_time - start_time}: Values Received != Values Generated")
        else:
            if current_time - loop_time > update_freq:
                print("Still no packets received - Time Passed")
                loop_time = time.time()

    with open('results.csv','w+') as out:
        csv_out= csv.writer(out)
        csv_out.writerow(['Time','Success'])
        for row in result_list:
            csv_out.writerow(row)
                


if __name__ == "__main__":
    random.seed(1275)
    print("Starting Time Before System Maintenance Test for the Hospital Station")
    TBSM_hospital_start(1)
    print("Time Before System Maintenance Test Complete")