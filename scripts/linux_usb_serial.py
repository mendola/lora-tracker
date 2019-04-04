from serial import Serial
from serial.serialutil import SerialException
import pdb

import sys
import select

# If there's input ready, do something, else do something
# else. Note timeout is zero so select won't block at all.
def nonblocking_read():
    while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
        line = sys.stdin.readline()
        if line:
            return line
        else: # an empty line means stdin has been closed
            return None


with Serial("/dev/ttyACM0", 115200) as ser:
    ser.flushInput()
    while True:
        if ser.inWaiting() > 0:
            newdata = ''
            while ser.inWaiting() > 0:
                raw_in = ser.read(1)
               # pdb.set_trace()
                try:
                    newdata = raw_in.decode("utf-8") 
                    print(newdata,end='')
                except UnicodeDecodeError:
                    pdb.set_trace()
            #handle_buffer(newdata)
            #print(newdata,end='')
        line_in = nonblocking_read()
        if line_in is not None:
            ser.write(line_in.encode())