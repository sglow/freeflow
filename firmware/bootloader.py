#!/usr/bin/python

import time
import serial
import sys
import utils
from elffile import ElfFile
from optparse import OptionParser

parser = OptionParser()
parser.add_option( "--serial", dest="port", help="Which serial port to use", default="/dev/ttyUSB0" );
parser.add_option( "--baud",   dest="baud", help="Serial port baud rate", default=115200 );
parser.add_option( "--verbose", dest="verbose", help='Get chatty', default=False );
(options, args) = parser.parse_args()


def main():
   ser = serial.Serial( port=options.port, baudrate=options.baud, parity='E' )
   ser.timeout = 0.5

   print 'Trying to connect to ARM...'
   Connect( ser )

   # Get boot loader version and list of supported commands
   info = SendCmd( ser, 0 )
   print 'Boot loader version: 0x%02x' % info[0]

   print 'Erasing flash'
   if( not Erase( ser, pages=range(48) ) ):
      print 'Erase failed'
      return

   elf = ElfFile( 'main.elf' )

   for s in elf.seg:
      if( len(s.data) < 1 ): continue;

      if( s.vaddr != 0x08000000 ):
         print 'Unexpected vaddr in segment, should be 0x08000000'
         print s
         return

      print 'Writing %d bytes to 0x%08x' % (len(s.data), s.vaddr)
      Poke( ser, s.vaddr, s.data )

   print 'Jumping to program'
   Go( ser, 0x08000000 )

def Connect( ser ):
   while( True ):
      ser.flush();
#      print 'sending'
      ser.write( chr(0x7f) )
      rsp = ser.read( 100 );
      if( len(rsp) == 1 and ord(rsp[0]) == 0x79 ):
         return True

      if( len(rsp) == 1 and ord(rsp[0]) == 0x1F ):
         return True

      for r in rsp:
         print '0x%02x' % ord(r),
      if( len(rsp) ): print
      time.sleep(.5)

def WaitACK( ser ):
   if( options.verbose ): print 'Waiting for ACK'
   ack = ser.read(1)
   if( len(ack) == 0 ):
      print 'Timeout waiting on ACK'
      return False
   ack = ord(ack[0])
   if( ack == 0x79 ):
      if( options.verbose ): 
         print 'ACK received'
      return True
   if( options.verbose ): 
      if( ack == 0x1F ):
         print 'NAK received'
      else:
         print 'Invalid ACK: 0x%02x' % ack
   return False;

def SerWrite( ser, msg ):
   if( options.verbose ): 
      print 'Sending:',
      for m in msg: print '0x%02x' % m,
      print

   msg = ''.join( [chr(x) for x in msg] )
   ser.write(msg)

def SendData( ser, dat ):
   cksum = 0;
   for x in dat: cksum ^= x
   dat.append( cksum & 0xff )
   SerWrite( ser, dat )
   return WaitACK( ser )

def SendCmd( ser, op, dat=None, rspLen=None, endACK=True ):
   ser.flush();
   cmd = [op, ~op & 0xff ]

   if( options.verbose ): 
      print
      print 'Sending command: 0x%02x' % op
   SerWrite( ser, cmd )

   if( not WaitACK( ser ) ):
      return None;

   if( dat != None ):
      if( not SendData( ser, dat ) ):
         return None

   if( not rspLen == None ):
      ct = rspLen
      if( ct == 0 ):
         return []
   else:
      ct = ser.read(1)
      if( not len(ct) ):
         print 'Failed to read count'
         return None
      ct = ord(ct[0]) + 1

   rsp = ser.read( ct );
   rsp = [ord(x) for x in rsp]

   if( options.verbose ): 
      print 'Received: '
      for c in rsp: print '0x%02x' % c,
      print

   if( len(rsp) != ct ):
      print 'Missing data in response, expected %d' % ct
      return None

   if( endACK and not WaitACK( ser ) ):
      return None;
   return rsp

def Peek( ser, addr, ct=1 ):
   dat = utils.Split32( addr, le=False );
   if( SendCmd( ser, 0x11, dat=dat, rspLen=0 ) == None ):
      return None
   rsp = ser.read( 100 );
   if( len(rsp) ):
      print 'Extra data before sending count',
      for r in rsp: print '0x%02x' % ord(r),
      print

   return SendCmd( ser, ct-1, rspLen=ct, endACK=False )

def Poke( ser, addr, data ):
   if( len(data) < 1 ): return

   while( len(data) > 256 ):
      ok = Poke( ser, addr, data[:256] );
      if( not ok ): return False
      addr += 256
      data = data[256:]

   dat = utils.Split32( addr, le=False );
   if( SendCmd( ser, 0x31, dat=dat, rspLen=0 ) == None ):
      return False

   dat = [len(data)-1] + data
   if( not SendData( ser, dat ) ):
      return False

   return True

def Erase( ser, pages=None, bank1=False, bank2=False ):
   # Check for mass erase
   if( pages == None ):
      dat = [0xFF, 0xFF]
   elif( bank1 ):
      dat = [0xFF, 0xFE]
   elif( bank2 ):
      dat = [0xFF, 0xFD]
   else:
      dat = utils.Split16( [len(pages)-1] + pages, le=False )

   ser.timeout = 20
   if( SendCmd( ser, 0x44, dat=dat, rspLen=0 ) == None ):
      ser.timeout = 0.5
      return False
   ser.timeout = 0.5
   return True

def Go( ser, addr ):
   dat = utils.Split32( addr, le=False );
   if( SendCmd( ser, 0x21, dat=dat, rspLen=0 ) == None ):
      return False
   return True



main()
