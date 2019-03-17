import serial
import json
import time

sent = False
buffert = 30

ser = serial.Serial('/dev/ttyACM0',9600)
syncmes = 'T{}n'.format(int(time.time()))
# had to send after monitoring
#ser.write(syncmes.encode("utf-8"))

t = 0
content = ""
while True:
    t += 1
    content = ser.readline().decode("utf-8")
    if not sent and t<buffert:
        print("sending:",syncmes)
        ser.write(syncmes.encode("utf-8"))
        sent = True
    print(content)
    if content[0] == "{":
        print("!!!!!!!!!!!!!!!!!!!!!")
        j = json.loads(content)
        print(j)
