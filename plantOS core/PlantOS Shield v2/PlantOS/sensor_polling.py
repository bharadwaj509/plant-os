#!/usr/bin/python

import io         # used to create file streams
import fcntl      # used to access I2C parameters like addresses
import time       # used for sleep delay and timestamps
import string     # helps parse strings
import smbus
import Adafruit_DHT

#
#   Atlas Scientific
#

class AtlasI2C:
    long_timeout = 1.5         	# the timeout needed to query readings and calibrations
    short_timeout = 0.5         	# timeout for regular commands
    default_bus = 1         	# the default bus for I2C on the newer Raspberry Pis, certain older boards use bus 0
    default_address = 99     	# the default address for the sensor
    current_addr = default_address
        
    def __init__(self, address=default_address, bus=default_bus):
        # open two file streams, one for reading and one for writing
        # the specific I2C channel is selected with bus
        # it is usually 1, except for older revisions where its 0
        # wb and rb indicate binary read and write
        self.file_read = io.open("/dev/i2c-"+str(bus), "rb", buffering=0)
        self.file_write = io.open("/dev/i2c-"+str(bus), "wb", buffering=0)
                
        # initializes I2C to either a user specified or default address
        self.set_i2c_address(address)
    
    def set_i2c_address(self, addr):
        # set the I2C communications to the slave specified by the address
        # The commands for I2C dev using the ioctl functions are specified in
        # the i2c-dev.h file from i2c-tools
        I2C_SLAVE = 0x703
        fcntl.ioctl(self.file_read, I2C_SLAVE, addr)
        fcntl.ioctl(self.file_write, I2C_SLAVE, addr)
        self.current_addr = addr
        
    def write(self, cmd):
        # appends the null character and sends the string over I2C
        cmd += "\00"
        self.file_write.write(cmd)
        
    def read(self, num_of_bytes=31):
        # reads a specified number of bytes from I2C, then parses and displays the result
        res = self.file_read.read(num_of_bytes)         # read from the board
        response = filter(lambda x: x != '\x00', res)     # remove the null characters to get the response
        if ord(response[0]) == 1:             # if the response isn't an error
            # change MSB to 0 for all received characters except the first and get a list of characters
            char_list = map(lambda x: chr(ord(x) & ~0x80), list(response[1:]))
            # NOTE: having to change the MSB to 0 is a glitch in the raspberry pi, and you shouldn't have to do this!
            if self.current_addr == 99: # pH sensor
                return "pH: " + ''.join(char_list)     # convert the char list to a string and returns it
            elif self.current_addr == 100: # EC sensor
                temp_string=''.join(char_list)
                print 'EC: ' + string.split(temp_string, ",")[0]
                print 'TDS: ' + string.split(temp_string, ",")[1]
                print 'Salinity: ' + string.split(temp_string, ",")[2]
                return "Gravity: " + string.split(temp_string, ",")[3]     # convert the char list to a string and returns it
            elif self.current_addr == 102: # RTD sensor
                return "Soluble Temperature: " + ''.join(char_list) + " C"     # convert the char list to a string and returns it
        else:
            return "Error " + str(ord(response[0]))
    
    def query(self, string):
        # write a command to the board, wait the correct timeout, and read the response
        self.write(string)
            
        # the read and calibration commands require a longer timeout
        if((string.upper().startswith("R")) or
            (string.upper().startswith("CAL"))):
            time.sleep(self.long_timeout)
        elif string.upper().startswith("SLEEP"):
            return "sleep mode"
        else:
            time.sleep(self.short_timeout)
                
        return self.read()
        
    def close(self):
        self.file_read.close()
        self.file_write.close()
        
    def list_i2c_devices(self):
        prev_addr = self.current_addr # save the current address so we can restore it after
        i2c_devices = []
        for i in range (0,128):
            try:
                self.set_i2c_address(i)
                self.read()
                i2c_devices.append(i)
            except IOError:
                pass
        self.set_i2c_address(prev_addr) # restore the address we were using
        return i2c_devices

#
#   BH1750
#

class BH1750():
    # define constants
    default_address = 0x23              # the default bus for I2C on the newer Raspberry Pis, certain older boards use bus 0
    default_bus = 1                     # the default address for the sensor
    POWER_DOWN = 0x00                   # No active state
    POWER_ON = 0x01                     # Power on
    RESET = 0x07                        # Reset data register value
    CONTINUOUS_LOW_RES_MODE = 0x13      # Start measurement at 4lx resolution. Time typically 16ms
    CONTINUOUS_HIGH_RES_MODE_1 = 0x10   # Start measurement at 1lx resolution. Time typically 120ms
    CONTINUOUS_HIGH_RES_MODE_2 = 0x11   # Start measurement at 0.5lx resolution. Time typically 120ms
    ONE_TIME_HIGH_RES_MODE_1 = 0x20     # Start measurement at 1lx resolution. Time typically 120ms. Device is automatically set to Power Down after measurement.
    ONE_TIME_HIGH_RES_MODE_2 = 0x21     # Start measurement at 0.5lx resolution. Time typically 120ms. Device is automatically set to Power Down after measurement.
    ONE_TIME_LOW_RES_MODE = 0x23        # Start measurement at 1lx resolution. Time typically 120ms. Device is automatically set to Power Down after measurement.
    
    def __init__(self):
        self.bus = smbus.SMBus(1)
    
    def convertToNumber(self, data):
        return ((data[1]+(256*data[0]))/1.2)

    def readLight(self, addr=default_address):
        data=self.bus.read_i2c_block_data(addr,self.ONE_TIME_HIGH_RES_MODE_1)
        return self.convertToNumber(data)

#
#   Main
#


def main():
    device = AtlasI2C() 	# creates the I2C port object, specify the address or bus if necessary
    device1 = BH1750()
    device2 = Adafruit_DHT.DHT22
    
    pin = 24 # DHT22 data pin
    
    # main loop
    while True:
        input = raw_input("Enter command: ")
                
        if input.upper().startswith("LIST_ADDR"):
            devices = device.list_i2c_devices()
            for i in range(len (devices)):
                print devices[i]
                
        # continuous polling command automatically polls the board
        elif input.upper().startswith("POLL"):
            delaytime = float(string.split(input, ',')[1])
                        
            # check for polling time being too short, change it to the minimum timeout if too short
            if delaytime < AtlasI2C.long_timeout:
                print("Polling time is shorter than timeout, setting polling time to %0.2f" % AtlasI2C.long_timeout)
                delaytime = AtlasI2C.long_timeout
                        
            # get the information of the board you're polling
            info = string.split(device.query("I"), ",")[1]
            print("Polling %s sensor every %0.2f seconds, press ctrl-c to stop polling" % (info, delaytime))
                        
            try:
                while True:
                    humidity, temperature = Adafruit_DHT.read_retry(device2, pin)
                    if humidity is not None and temperature is not None:
                        print('Ambient Temperature: {0:0.1f} C  \nAmbient Humidity: {1:0.1f} %'.format(temperature, humidity))
                    else:
                        print('Failed to get reading DHT22. Try again!')
                    device.set_i2c_address(99)
                    print(device.query("R"))    # (99 pH)
                    device.set_i2c_address(100)
                    print(device.query("R"))    # (100 EC)
                    device.set_i2c_address(102)
                    print(device.query("R"))    # (102 RTD)
                    print 'Light Intensity: ' + str(device1.readLight()) + ' lx'
                    time.sleep(delaytime - AtlasI2C.long_timeout)
            except KeyboardInterrupt: 		# catches the ctrl-c command, which breaks the loop above
                    print("Continuous polling stopped")
                        
        # if not a special keyword, pass commands straight to board
        else:
            if len(input) == 0:
                print "Please input valid command."
            else:
                try:
                    print(device.query(input))
                except IOError:
                    print("Query failed \n - Address may be invalid, use List_addr command to see available addresses")


if __name__ == '__main__':
    main()
