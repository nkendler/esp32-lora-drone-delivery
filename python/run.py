import argparse
import pandas as pd
import numpy as np
import time
import os

class SheetParser():
    def __init__(self, sheet_path=None):
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


if __name__ == "__main__":

    sheet_path= "Order.xlsx"

    #Going to turn into an argument
    sp = SheetParser(sheet_path)

    sp.display_data()