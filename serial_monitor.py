import serial
import json

ser = serial.Serial('/dev/ttyACM0',9600)

content = ""
while True:
    content = ser.readline().decode("utf-8")
    print(content)
    if content[0] == "{":
        print("!!!!!!!!!!!!!!!!!!!!!")
        j = json.loads(content)
        print(j)

