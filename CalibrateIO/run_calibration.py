#
# run_calibration.py
# 
# Chip Audette, OpenAudio, Oct 2024
# MIT License 
#
# This script communicates with the Tympan over the USB Serial link.
# The purpose is to run the stepped-tone calibration and to retrieve
# the results
#
# I'm using this as an example: https://projecthub.arduino.cc/ansh2919/serial-communication-between-python-and-arduino-663756
#

import serial  #pip install pyserial
import time 
import matplotlib.pyplot as plt
import codecs
import numpy as np

# ##################### Define functions
def clearSerialBuffer():
    foo = getReply(False,1.0)

def printReceivedLine(all_lines):
    #if (len(all_lines) > 0) and (all_lines[-1]=='\n'):
    #    all_lines = all_lines[:-1] #strip off trailing \n
    #if (len(all_lines) > 0) and (all_lines[-1]=='\r'):
    #    all_lines = all_lines[:-1] #strip off trailing \r
    print(all_lines,end='')

def getReply(print_as_received=True,wait_period_sec=0.5):
    all_lines = ''
    new_readline = codecs.decode(serial_with_tympan.readline(),encoding='utf-8')
    all_lines += new_readline
    last_reply_time  = time.time()
    #while (len(new_readline) > 0):
    while (time.time() < (last_reply_time + wait_period_sec)):
        new_readline = codecs.decode(serial_with_tympan.readline(),encoding='utf-8')
        if len(new_readline) > 0:
            last_reply_time = time.time()
        all_lines += new_readline
        if print_as_received:
            printReceivedLine(new_readline)
    return all_lines

def sendCharacterAndGetResponse(send_character, print_as_received = True, wait_period_sec = 0.5):
    serial_with_tympan.write(bytes(send_character + '\n', 'utf-8'))  #send an 'h' to the Tympan
    time.sleep(0.05)                             #wait a bit to enable a response 
    all_lines = getReply(print_as_received, wait_period_sec)
    return all_lines

def parseTestDataString(all_lines):
    all_vals = []
    list_strings = all_lines.splitlines()  #convert long string (with newlines) into list of strings
    test_string = 'dBFS):'
    for line in list_strings:
        ind = line.find(test_string)
        if  ind != -1:
            ind += len(test_string)
            line = line[ind:]
            vals = np.fromstring(line,dtype=float,sep=',')
            all_vals.append(vals)
    all_vals = np.array(all_vals)
    test_id = all_vals[:,0]
    freq_Hz = all_vals[:,1]
    input_dBFS = all_vals[:,2:]
    return test_id, freq_Hz, input_dBFS



# #################33 Here is the Main

#specify the COM port of the Tympan
my_com_port = 'COM26'  #Look in the Arduino IDE! 

# create a serial instance for communicating to our device
print("Opening serial port...make sure the Serial Monitor is closed in Arduino IDE")
serial_with_tympan = serial.Serial(port=my_com_port, baudrate=115200, timeout=.5)

# mute the system to stop any curren test
all_lines = sendCharacterAndGetResponse('m',print_as_received=False, wait_period_sec=0.5)

# reset the test parameters
all_lines = sendCharacterAndGetResponse('q')

# ask the Tympan for the help menu and read the response
all_lines = sendCharacterAndGetResponse('h')

wait_period_sec = 1.1  # default value for slow (1 second)
if 1:
    # speed up the test by shorting from the default 0.5sec/step to 0.2 sec/step the test parameters
    all_lines = sendCharacterAndGetResponse('DDD')
    wait_period_sec = 0.25  #faster value for faster (0.2 sec) test tones


if 1:
    # command the test to start
    all_lines = sendCharacterAndGetResponse('T',wait_period_sec=wait_period_sec)

    # get all of the results
    all_lines = sendCharacterAndGetResponse('v')

    # parse the data
    test_id, freq_Hz, input_dBFS = parseTestDataString(all_lines)

    # plot
    plt.semilogx(freq_Hz,input_dBFS,linewidth=2)
    plt.legend(['Left','Right'])
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Input Level (dBFS)')



# close the serial port
print("Closing serial port...")
serial_with_tympan.close()

