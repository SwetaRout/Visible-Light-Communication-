import serial
import sys
from socket import *
import struct

bytesToRead = 50

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

ack = struct.Struct('B')

#setup serial communication with arduino
ser = serial.Serial('/dev/cu.usbmodem1411', 115200, timeout=None, xonxoff=False, rtscts=False, dsrdtr=False)
ser.flushInput()
ser.flushOutput()

#setup communication to raspberry PI
serverPort = 12344
serverSocket = socket(AF_INET,SOCK_STREAM)
serverSocket.bind(('APL-C02SD0YHG8WM.local', serverPort))
serverSocket.listen(5)
print ('Ready to connect to raspberry pi')
connectionSocket, addr = serverSocket.accept()
var = raw_input("\nPress 1 for console input.\nPress 2 for file input.\n")

if(var == '1'): #Console entry
    while True:
       data = ser.read(bytesToRead)
       sys.stdout.write(str(data[4]))
       sys.stdout.flush()
       checksum = crcKey(ord(data[4]))

       sentChecksum = ord(data[3]) << 8
       sentChecksum |= ord(data[2])

       if checksum == sentChecksum:
          ack = '1'
       else:
          ack = '0'
       connectionSocket.send(ack)

elif(var == '2'): #file transfer
   f = open('received_file.zip','wb')
   while True:
      print('receiving data...')
      data = ser.read(bytesToRead)
      fileData = ''.join(data[8:])
      checksum = crc16(fileData)

      sentChecksum = ord(data[7]) << 24
      sentChecksum |= ord(data[6]) << 16
      sentChecksum |= ord(data[5]) << 8
      sentChecksum |= ord(data[4])

      if checksum == sentChecksum:
          ack = '1'
      else:
          ack = '0'
      connectionSocket.send(ack)


      # write data to a file
      f.write(fileData)
      dataSize = ord(data[3]) << 24
      dataSize |= ord(data[2]) << 16
      dataSize |= ord(data[1]) << 8
      dataSize |= ord(data[0])
      if(dataSize < 42):
          break

   f.close()
   print('Successfully get the file')
   while(True):
       pass
   s.close()
   print('connection closed')
