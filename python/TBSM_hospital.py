from serial import Serial
import serial.tools.list_ports as ports
import os
import sys
import time, serial
from processors import OrderReceiver, SheetParser

if __name__ == "__main__":
    print("Starting Time Before System Maintenance Test for the Hospital Station")