import serial
import json
import time

sent = False

ser = serial.Serial('/dev/ttyACM0',9600)
syncmes = 'T{}n'.format(int(time.time()))
print("sending:",syncmes)
# had to send after monitoring
#ser.write(syncmes.encode("utf-8"))

content = ""
while True:
    content = ser.readline().decode("utf-8")
    if not sent:
        ser.write(syncmes.encode("utf-8"))
        sent = True
    print(content)
    if content[0] == "{":
        print("!!!!!!!!!!!!!!!!!!!!!")
        j = json.loads(content)
        print(j)
