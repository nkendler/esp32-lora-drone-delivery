
## Our ESP-12F Pin Assignments
HIGH: VCC, EN

LOW: GND, GPIO15, 

SDA (OLED): GPIO4

SCL (OLED): GPIO5

(Serial to computer): RXD, TXD

Program (toggle): GPIO0

Reset (button): RST

## ESP-12F Map
. | . | Antenna | on this | side! | :) | . | . 
--- | --- | --- | --- | --- | --- | --- | ---
1 RST | . | . | . | . | . | . | 22 TXD
2 ADC | . | . | . | . | . | . | 21 RXD
3 EN | . | . | . | . | . | . | 20 GPIO5
4 GPIO16 | . | . | . | . | . | . | 19 GPIO4
5 GPIO14 | . | . | . | . | . | . | 18 GPIO0
6 GPIO12 | . | . | . | . | . | . | 17 GPIO2
7 GPIO13 | . | . | . | . | . | . | 16 GPIO15
8 VCC | . | . | . | . | . | . | 15 GND
. | 9 CS0 | 10 MISO | 11 IO9 | 12 IO10 | 13 MOSI | 14 SCLK | .


## ESP-12F Pin Descriptions
\# | Pin | Description
--- | --- | ---
1 | RST | Reset the module 
2 | ADC | A/D Conversion result.Input voltage range 0-1v,scope:0-1024 
3 | EN | Chip enable pin. Active high 
4 | GPIO16 | GPIO16;    can be used to wake up the chipset from deep sleep mode 
5 | GPIO14 | GPIO14; HSPI_CLK 
6 | GPIO12 | GPIO12; HSPI_MISO 
7 | GPIO13 | GPIO13; HSPI_MOSI; UART0_CTS 
8 | VCC | 3.3V power supply (VDD) 
9 | CS0 | Chip selection 
10 | MISO | Salve output Main input 
11 | IO9 | GPIO9 
12 | IO10 | GBIO10 
13 | MOSI | Main output slave input 
14 | SCLK | Clock 
15 | GND | GND 
16 | GPIO15 | GPIO15; MTDO; HSPICS; UART0_RTS 
17 | GPIO2 | GPIO2; UART1_TXD 
18 | GPIO0 | GPIO0 
19 | GPIO4 | GPIO4 
20 | GPIO5 | GPIO5 
21 | RXD | UART0_RXD; GPIO3 
22 | TXD | UART0_TXD; GPIO1
