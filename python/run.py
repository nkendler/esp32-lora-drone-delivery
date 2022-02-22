import argparse
import pandas as pd
import numpy as np
import time
import os
import sys

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
        self.sheet_df = None
        if sheet_path is not None:
            self.import_sheet(sheet_path)

    def import_sheet(self, sheet_path):
        try:
            self.sheet_df = pd.read_excel(sheet_path)
            print(f"Spreadsheet Succesfully Loaded")
        except:
            print(f"No Spreadsheet at Location: {sheet_path}")

    def display_data(self):
        if self.sheet_df is not None:
            print(self.sheet_df)
        else:
            print("NO SPREADSHEET LOADED")

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
            sys.exit("Invalid range for Disctrict Hospital. Please pick 0 or 1.")

        if (self.RuralStation > 49 or self.RuralStation < 0):
            sys.exit("Invalid range for Rural Station. Please pick a value between 0 and 49.")

        sum = self.ItemOne + self.ItemTwo + self.ItemThree + self.ItemFour + self.ItemFive + self.ItemSix
        if (sum > 24):
            sys.exit("Invalid quantities. Please reselect to have quantity less than or equal to 24.")

        print("Data is valid")


if __name__ == "__main__":

    sheet_path= "Order.xlsx"

    #Going to turn into an argument
    sp = SheetParser(sheet_path)

    sp.display_data()
    sp.parse_data()
    sp.check_valid()