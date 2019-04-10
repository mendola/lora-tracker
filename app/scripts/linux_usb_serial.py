from serial import Serial
from serial.serialutil import SerialException
import sys
import select
import struct
from maps_integration import fetch_map
import matplotlib.pyplot as plt
from matplotlib import animation
import PIL.Image
import signal
import threading

current_map = None
current_coordinates = (b'',b'')
global map_needs_updating
map_needs_updating = False
STOP = False

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

def usb_interface():
    global current_coordinates
    global map_needs_updating
    try:
        with Serial("/dev/ttyACM0", 115200) as ser:
            ser.flushInput()
            while not STOP:
                if ser.inWaiting() > 0: # if data in serial buffer
                    newdata = ''
                    while ser.inWaiting() > 0: # while there's still data in serial buffer
                        raw_in = ser.read(1)  # read one binary character
                        ret = handle_raw_char(raw_in)
                        if ret is not None:
                            latitude_degrees = ret[0]
                            longitude_degrees = ret[1]
                            print("Node Coordinates are: %f, %f" % (latitude_degrees, longitude_degrees))
                            current_coordinates = (latitude_degrees, longitude_degrees)
                            map_needs_updating = True
                line_in = nonblocking_read()  # nonblocking read from keyboard
                if line_in is not None:
                    ser.write(line_in.encode())  # Send keyboard input over serial
    except Exception:
        print("Stopping USB thread.")

def map_loop():
    global current_map
    global current_coordinates
    global map_needs_updating
    try:
        current_map = PIL.Image.open('current_map.png')
        print('got the current map!')
    except IOError:
        print('makin a new map')
    # default coordinates to NEU ~
        fetch_map(42.338958, -71.0880675)
        current_map = PIL.Image.open('current_map.png')

    print("Starting map loop!!!")
    fig = plt.figure()
    ax = fig.add_subplot(1,1,1)
    im = ax.imshow(current_map, animated=True)
    def update_map(i):
        global map_needs_updating
        if map_needs_updating:
            fetch_map(current_coordinates[0], current_coordinates[1])
            map_needs_updating = False
            current_map = PIL.Image.open('current_map.png')
            im.set_array(current_map)
    ani = animation.FuncAnimation(fig, update_map, interval=1)
    plt.show()


serial_thread = threading.Thread(target=usb_interface)
serial_thread.daemon = False
serial_thread.start()
plotting_thread = threading.Thread(target=map_loop)
plotting_thread.daemon = False
plotting_thread.start()

# usb_interface()
# map_loop()

def stop(a, b):
    global STOP
    STOP = True
    raise Exception

signal.signal(signal.SIGINT, stop)
signal.signal(signal.SIGTERM, stop)
