import serial
import time
import keyboard
import json
import math
import serial.tools.list_ports

# set up the serial line

ports = list(serial.tools.list_ports.comports())
ard_port = ""
for p in ports:
    if ("CH340" in p.description):
        ard_port = p[0]

ser = serial.Serial(ard_port, 9600)
ser.flushInput()
time.sleep(2) #give the connection a second to settle

# set up key binding settings
settings = {}
with open("settings.json", "r") as read_file:
    settings = json.load(read_file)


# split string into 64 bytes
charLen = 63
split = []
unsplit = json.dumps(settings) + "<e>"
passes = math.ceil(len(unsplit) / charLen)
for x in range(int(passes)):
    if (x == passes): 
        split.append(unsplit[x*charLen:])
    else:
        split.append(unsplit[x*charLen:((x+1)*charLen)])
# pass in new settings to board
for section in split:
    ser.write(str.encode(section))
    time.sleep(1)

# convert serial to key combo
def serial_to_press(command):    
    try:
        keyboard.press_and_release(command)
    except:
        print("unknown command")

# listen to keypresses
try: 
    while True:
        buffer = ser.in_waiting
        if (buffer > 0):
            # detect if valid command
            cmd = ser.readline().decode("UTF-8")
            print(cmd)
            compare = cmd[:4]
            if (compare == "cow>"):
                cmd = cmd.replace("cow>", "")
                cmd = cmd.replace("\r\n", "")
                serial_to_press(cmd)
except KeyboardInterrupt:
    print("Keyboard Interrupt")
    # exit()
