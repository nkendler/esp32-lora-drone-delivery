from turtle import bye


bit_packet = b'10100000000100010100011000010001001'
int_packet = int(bit_packet,2)
hex_packet = hex(int_packet)

print(bit_packet)
print(int_packet)
print(hex_packet)


byte_array = (int_packet).to_bytes(5, byteorder = 'big')
print("The bytes are : ", byte_array)
print(f"Now in list form: {list(byte_array)}")

district_hospital = 1
rural_station = 25
item_one = 22
item_two = 10
item_three = 5
item_four = 1
item_five = 19
item_six = 12

packet = item_six
print(f"Packet After Item 6 Addition: {bin(packet)}")
packet = packet << 5
packet += item_five
print(f"Packet After Item 5 Addition: {bin(packet)}")
packet = packet << 5
packet += item_four
print(f"Packet After Item 4 Addition: {bin(packet)}")
packet = packet << 5
packet += item_three
print(f"Packet After Item 3 Addition: {bin(packet)}")
packet = packet << 5
packet += item_two
print(f"Packet After Item 2 Addition: {bin(packet)}")
packet = packet << 5
packet += item_one
print(f"Packet After Item 1 Addition: {bin(packet)}")
packet = packet << 6
packet += rural_station
print(f"Packet After Item Rural Station Addition: {bin(packet)}")
packet = packet << 1
packet += district_hospital

print(f"Encoded Packet: {bin(packet)}")
print(f"Encoded Packet: {hex(packet)}")