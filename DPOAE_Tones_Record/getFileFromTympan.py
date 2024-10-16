#
# getFileFromTympan.py
# 
# Chip Audette, OpenAudio, Oct 2024
# MIT License 
#
# This script communicates with the Tympan over the USB Serial link.
# The purpose is to send a file from the Tympan's SD card over to the
# PC and saved on the PC's local disk
#
# I'm using this as a Serial communication example: 
# https://projecthub.arduino.cc/ansh2919/serial-communication-between-python-and-arduino-663756
#

import serial  #pip install pyserial
import tympanSdFileTransferFunctions as tympanSerial


# VERY IMPORTANT: specify the COM port of your Tympan. You can Look in the Arduino IDE to find this.
my_com_port = 'COM26'    #YOUR INPUT IS NEEDED HERE!!!  


# Do you want to learn what is happening?  Or, do you need to debug?
verbose = False   #set to true for printing of helpful info


# create a serial instance for communicating to your Tympan
print("ACTION: Opening serial port...make sure the Serial Monitor is closed in Arduino IDE...")
if ('serial_to_tympan' in locals()): serial_to_tympan.close()  #close serial port if already open
wait_period_sec = 0.5  #how long before serial comms time out (could set this faster, if you want)
serial_to_tympan = serial.Serial(port=my_com_port, baudrate=115200, timeout=wait_period_sec) #baudrate doesn't matter for Tympan
print("RESULT: Serial port opened successfully")


# [Optional] let's test the connection by asking for the help menu
print();print('ACTION: Requesting the Tympan Help Menu...')
tympanSerial.sendTextToSerial(serial_to_tympan, 'h')                   #send the command to the Tympan
reply = tympanSerial.readMultipleLinesFromSerial(serial_to_tympan)     #get the mutli-line reply from the Tympan
print("REPLY:",reply.strip()); print()



# ############# Let's see what files are on the SD Card

# Check to see what files are on the SD
print();print("ACTION: Reading the file names on the Tympan SD Card...")
command_getFilenames = 'z'
tympanSerial.sendTextToSerial(serial_to_tympan, command_getFilenames)  #send the command to the Tympan
reply = tympanSerial.readLineFromSerial(serial_to_tympan)              #get the mutli-line reply from the Tympan
print("REPLY:",reply.strip())  

# let's break up the full text reply into the filenames
fnames = tympanSerial.processLineIntoFilenames(reply)     #parse out the filenames
print("RESULT: Files on Tympan SD:", fnames)              #print the line to the screen here in Python

# ############# Now, Let's pick which file to download

# choose the file that you want to download to the PC
if 0:  #set to 1 (ie, true) to use the first option or set to 0 (ie, false) to use the second option
    # explicitly specify which file you want from the SD
    fname_to_read_on_Tympan = 'AUDIO001.WAV' 
else:
    #or, choose the target file based on the file names reported by the Tympan
    wav_fnames = tympanSerial.keepFilenamesOfType(fnames,targ_types=['wav'])  #look just at WAV files
    fname_to_read_on_Tympan = wav_fnames[-1]    #let's load the last one (ie, the most recent?) 


# Transfer a file FROM THE TYMPAN
print();print("ACTION: Receiving the file " + fname_to_read_on_Tympan + " from the Tympan...")
command_getFileFromTypman = 'x'                     #This is set by the Tympan program
fname_to_write_locally = fname_to_read_on_Tympan    #on the local computer, what file to write to?  ...use the same as the source name
receive_success = tympanSerial.receiveFileFromTympan(serial_to_tympan, command_getFileFromTypman, \
                        fname_to_read_on_Tympan, fname_to_write_locally, verbose=verbose)


if (receive_success):
    print("RESULT: File " + fname_to_read_on_Tympan + " received successfully!  Saved locally as " + fname_to_write_locally)
else:
    print("RESULT: File " + fname_to_read_on_Tympan + " failed to be received from the Tympan")


# Finally, let's finish up
print(); print("ACTION: Cleaning up...")
serial_to_tympan.close()
print("Result: Serial port closed")

