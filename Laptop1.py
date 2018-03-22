import struct
import os
import zipfile
import sys
import curses
from socket import *
import random
import time

#CRC function
def crc16(data_in):
    poly = 0xB9B1
    size = len(data_in)
    crc = 0
    i = 0
    while i < size:
        byte = ord(data_in[i])
        j = 0
        while j < 8:
            if (crc & 1) ^ (byte & 1):
                crc = (crc >> 1) ^ poly
            else:
                crc >>= 1
            byte >>= 1
            j += 1
        i += 1
    return crc

#crc for key
def crcKey(data_in):
    poly = 0xB9B1
    size = 1
    crc = 0
    i = 0
    while i < size:
        byte = data_in
        j = 0
        while j < 8:
            if (crc & 1) ^ (byte & 1):
                crc = (crc >> 1) ^ poly
            else:
                crc >>= 1
            byte >>= 1
            j += 1
        i += 1
    return crc

def zip(src, dst):
	zf = zipfile.ZipFile("%s.zip" % (dst), "w")
	zf.write(src)
	zf.close()

keyFrame = struct.Struct('2BHB')
fileFrame = struct.Struct('I I 42s')
ack = struct.Struct('2I')
keyFrame_number = 1
end = 0

ENTER_KEY = (curses.KEY_ENTER, ord('\n'), ord('\r'))

#Setup socket
serverPort = 12345
serverSocket = socket(AF_INET,SOCK_STREAM)
serverSocket.bind(('APL-C02SD0YHG8WM.local', serverPort))
serverSocket.listen(5)
print ('The server is ready to connect')
connectionSocket, addr = serverSocket.accept()
var = raw_input("\nPress 1 for console input.\nPress 2 for file input.\n")
if(var == '1'):
    print("Enter STOP to end")
    print("Start ...")

    # Initialize the terminal
    win = curses.initscr()

    curses.cbreak()

    # Make getch() non-blocking
    win.nodelay(True)

    while True:
        # Turn off line buffering
        key = win.getch()
        time.sleep(0.01)
        # Only 8 bytes of input can be checked and processed at a time. This is for faster and better performance
        checksum = crcKey(key)

		#pack and send data
        if(key != -1):
           values = (keyFrame_number, keyFrame.size, checksum, key)
           keyFrame_out = keyFrame.pack(*values)
           connectionSocket.send(keyFrame_out)

           #wait for ACK msg before moving on to next character
           data = connectionSocket.recv(1)
           if data == '0':
               print("NACK")
        elif (key == ENTER_KEY):
           break


elif(var == '2'):
   filename=raw_input("Enter file name:")
   zip(filename, "sending_file")
   f = open('sending_file.zip','rb')
   l = f.read(42)
   while (l):
       checksum=crc16(l)

       size=sys.getsizeof(checksum)+len(l)
       values = (len(l), checksum, l)
       fileFrame_out = fileFrame.pack(*values)
       connectionSocket.send(fileFrame_out)
       l = f.read(42)
       data = connectionSocket.recv(1)
       if data == '0':
            print("File transfer failed")
            break
   f.close()
   os.remove('sending_file.zip')

   print('Done sending')

   while(True):
       pass
