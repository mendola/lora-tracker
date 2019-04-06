from serial import Serial
from serial.serialutil import SerialException
import pdb

import sys
import select
import struct
from maps_integration import plot_on_map

mini_gprmc_msg = None
latitude = b''
longitude = b''
totalbytes = None

# If there's input ready, do something, else do something
# else. Note timeout is zero so select won't block at all.
def nonblocking_read():
    while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
        line = sys.stdin.readline()
        if line:
            return line
        else: # an empty line means stdin has been closed
            return None


def handle_raw_char(char):
    global mini_gprmc_msg
    global latitude
    global longitude
    global totalbytes
    ret = None
    #print(char)
    if totalbytes is None:
        try:
            newdata = char.decode("utf-8")
            if newdata == '$':
                totalbytes = 0
            else:
                print(newdata,end='')
        except UnicodeDecodeError:
            print("Fucc")
    else:
        if totalbytes < 4:
            latitude += char
            totalbytes += 1
        elif totalbytes < 8:
            longitude += char
            totalbytes += 1
        else:
            if char == '\r'.encode('utf-8'):
                latitude_mm = struct.unpack('<L',latitude)[0]
                if latitude_mm & 0x80000000 != 0:
                    latitude_mm = -(latitude_mm - 0x80000000)
                longitude_mm = struct.unpack('<L',longitude)[0]
                if longitude_mm & 0x80000000 != 0:
                    longitude_mm = -(longitude_mm - 0x80000000)
                ret = latitude_mm / 60000, longitude_mm / 60000
                latitude = b''
                longitude = b''
                totalbytes = None
            else:
                totalbytes = None
                latitude = b''
                longitude = b''

    return ret

def handle_minigprmc_msg(msg):
    fields = msg.split(',')
    if len(fields < 6):
        return None
    else:
        return fields[0]

with Serial("/dev/ttyACM0", 115200) as ser:
    ser.flushInput()
    while True:
        if ser.inWaiting() > 0: # if data in serial buffer
            newdata = ''
            while ser.inWaiting() > 0: # while there's still data in serial buffer
                raw_in = ser.read(1)  # read one binary character
               # pdb.set_trace()
                ret = handle_raw_char(raw_in)
                if ret is not None:
                    latitude_degrees = ret[0]
                    longitude_degrees = ret[1]
                    print("Node Coordinates are: %f, %f" % (latitude_degrees, longitude_degrees))
                    plot_on_map(latitude_degrees, longitude_degrees)

        line_in = nonblocking_read()  # nonblocking read from keyboard 
        if line_in is not None:
            ser.write(line_in.encode())  # Send keyboard input over serial
