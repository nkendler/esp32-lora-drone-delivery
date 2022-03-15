import os
import sys
import time
from processors import OrderReceiver, SheetParser
from PyQt5.QtWidgets import (QApplication, QComboBox, QFileDialog, QLineEdit, QPushButton,
                             QStackedLayout, QVBoxLayout, QHBoxLayout, QWidget, QTextEdit)
from PyQt5.QtCore import QTimer


class Window(QWidget):
    def __init__(self):
        super().__init__()

        self.sp = SheetParser()
        self.ord = OrderReceiver()

        self.setWindowTitle("Aerlift Network Application")
        self.windowHeight, self.windowWidth = 800, 1200
        self.setFixedSize(self.windowWidth, self.windowHeight)
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        self.pageSelect = QComboBox()
        self.pageSelect.addItems(["Ground Station", "Hospital Station"])
        self.pageSelect.activated.connect(self.switchPage)
        self.stackedLayout = QStackedLayout()
        
        self.ground_station = self.ground_maker()
        self.hospital_station = self.hospital_maker()
        self.stackedLayout.addWidget(self.ground_station)
        self.stackedLayout.addWidget(self.hospital_station)

        self.layout.addWidget(self.pageSelect)
        self.layout.addLayout(self.stackedLayout)
    
    def switchPage(self):
        self.stackedLayout.setCurrentIndex(self.pageSelect.currentIndex())
    
    def console_maker(self):
        logOutput = QTextEdit()
        logOutput.setReadOnly(True)
        logOutput.setLineWrapMode(QTextEdit.NoWrap)

        font = logOutput.font()
        font.setFamily("Courier")
        font.setPointSize(10)
        return logOutput

    def ground_maker(self):
        ground_widget = QWidget()
        self.ground_line = QLineEdit()
        self.ground_console = self.console_maker()
        self.upload_btn = QPushButton("Upload Order", clicked=self.getfile)
        self.clear_btn = QPushButton("&CLEAR", clicked=self.ground_console.clear)
        self.ground_line.returnPressed.connect(self.on_line_edit_enter)
        
        
        upper_layout = QVBoxLayout()
        layout_top = QVBoxLayout()
        layout_bot = QHBoxLayout()
        
        layout_top.addWidget(self.ground_console)
        layout_bot.addWidget(self.ground_line, 7)
        layout_bot.addWidget(self.upload_btn, 2)
        layout_bot.addWidget(self.clear_btn, 1)
        

        upper_layout.addLayout(layout_top)
        upper_layout.addLayout(layout_bot)

        ground_widget.setLayout(upper_layout)
        return ground_widget

    def on_line_edit_enter(self):
        line_value = self.ground_line.text()
        self.ground_line.clear()
        ret = self.sp.import_sheet(line_value)
        if ret:
            self.ground_console.append("Succesfully Imported Spreadsheet")
        data_str = self.sp.display_data()
        self.ground_console.append(data_str)
        self.sp.parse_data()
        valid_check = self.sp.check_valid()
        self.ground_console.append(valid_check[1])
        if valid_check[0]:
            encoding = self.sp.encode_to_packet()
            self.ground_console.append(encoding)
    
    def getfile(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file', 
            'C:\\Users\\Ben\\Documents\\crapstone\\python',"Excel Files (*.xlsx)")
        self.ground_line.clear()
        self.ground_line.insert(fname[0])
        self.on_line_edit_enter()
    
    def hospital_maker(self):
        
        hospital_widget = QWidget()
        self.hospital_start_button = QPushButton("START", clicked=self.start_hospital_loop)
        self.hospital_stop_button = QPushButton("STOP", clicked=self.end_hospital_loop)
        self.hospital_console = self.console_maker()
        self.hospital_stop_button.setEnabled(False)
        
        upper_layout = QVBoxLayout()
        layout_top = QVBoxLayout()
        layout_bot = QHBoxLayout()
        
        layout_top.addWidget(self.hospital_console)
        layout_bot.addWidget(self.hospital_start_button, 5)
        layout_bot.addWidget(self.hospital_stop_button, 5)

        upper_layout.addLayout(layout_top)
        upper_layout.addLayout(layout_bot)

        hospital_widget.setLayout(upper_layout)
        return hospital_widget

    def end_hospital_loop(self):
        self.hospital_timer.stop()
        del self.hospital_timer
        self.hospital_console.append("Hospital Loop Ended")

        self.hospital_start_button.setEnabled(True)
        self.hospital_stop_button.setEnabled(False)

    def start_hospital_loop(self):
        self.hospital_timer = QTimer()
        self.hospital_timer.timeout.connect(self.hospital_loop_action)
        self.hospital_timer.start(1000)
        self.hospital_start_time = time.time()

        self.hospital_start_button.setEnabled(False)
        self.hospital_stop_button.setEnabled(True)
    
    def hospital_loop_action(self):
        self.hospital_console.append(f"Time Passed: {time.time() - self.hospital_start_time:.2f}")
        df = self.ord.format_packet_into_df()
        if df is not None:
            self.hospital_console.append(f"\nPacket Recieved")
            for column in df.columns:
                self.hospital_console.append(f"{column}: {df[column][0]}")
            self.hospital_console.append(f"Exported to: {self.ord.export_dataframe(df)}")
            self.hospital_console.append("")
            
        else:
            self.hospital_console.append(f"No Packet to Read")
        #Here we loop it for however frequently we want to acquire info from the device itself. Not sure how that would look

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