/* sercmd.c */

// This module handles receiving commands from the serial port and
// sending responses.  It's polled by the background loop.
//
// There are two command modes that we support, ASCII mode (the default on startup)
// and binary mode.
//
// In ASCII mode, commands are terminated with either a character return or line feed character.
// In binary mode, two special byte values are used; an end of command character (EOC) and an escape
// character (ESC).  Commands are terminated with the EOC.
// If an EOC or ESC byte needs to be sent as a data byte, then it's proceeded with ESC.
// For example, to send ESC you would send ESC ESC.  To send EOC you would send ESC EOC.
//
// The serial port starts in ASCII mode at power-up.

#include "ascii.h"
#include "binary.h"
#include "cpu.h"
#include "sercmd.h"
#include "string.h"
#include "uart.h"
#include "utils.h"

// Special binary mode bytes
#define EOC                     0xF1
#define ESC                     0xF2

// Flag bits
#define FLG_BINARY              0x01   // Set when running in binary mode
#define FLG_ESC                 0x02   // Set if the last byte was an ESC

// States
#define START_NEW_CMD           0
#define STATE_WAIT_ASCII        1
#define STATE_WAIT_BINARY       2
#define STATE_SEND_ASCII_RSP    3
#define STATE_SEND_BINARY_RSP   4

// local data
static uint8_t cmdBuff[200];
static int8_t state;
static uint8_t flag;
static int16_t cmdNdx;
static int16_t rspLen;

void SetDbg( int x )
{
if( x == dbgInt[0] ) return;
dbgInt[0] = x;

if( x&1 ) GPIO_SetPin( DIGIO_B_BASE, 6 );
else      GPIO_ClrPin( DIGIO_B_BASE, 6 );

if( x&2 ) GPIO_SetPin( DIGIO_B_BASE, 7 );
else      GPIO_ClrPin( DIGIO_B_BASE, 7 );
}

// This function is constantly called from the background task.
// It reads bytes from the UART and detects the end of a command.
// When a new command has been received, it passes it to the command
// processor and then sends the response back out the UART
void PollSerCmd( void )
{
// temp
flag |= FLG_BINARY;
   switch( state )
   {
      // Start of a new command.
      // In ASCII mode we discard any white space at the start of a command.
      // In binary mode we just jump to the next state
      case START_NEW_CMD:
         cmdNdx = 0;
         if( flag & FLG_BINARY )
            state = STATE_WAIT_BINARY;
         else
         {
            // Read the next character.  This will be -1 if there
            // aren't any new characters available
            int ch = UART_Recv();
            if( ch < 0 )
               return;

            // Discard white space
            if( strchr( " \n\r\t", ch ) )
               return;

            // For non white space, add to my command buffer
            // and move on to the next state
            cmdBuff[0] = ch;
            cmdNdx = 1;
            state = STATE_WAIT_ASCII;
         }
         return;

      // Reading bytes from the UART until the end of a command is reached.
      // ASCII mode
      case STATE_WAIT_ASCII:
      {
         int ch = UART_Recv();
         if( ch < 0 ) return;

         if( strchr( "\n\r", ch ) )
         {
            cmdBuff[ cmdNdx ] = 0;

            rspLen = ProcessAsciiCmd( (char*)cmdBuff, sizeof(cmdBuff) );
            cmdNdx = 0;
            state = STATE_SEND_ASCII_RSP;
            return;
         }

         if( cmdNdx < sizeof(cmdBuff) )
            cmdBuff[ cmdNdx++ ] = ch;
         return;
      }

      // Sending a response to an ASCII command
      case STATE_SEND_ASCII_RSP:
      {
         // Add as many bytes as possible to the UART 
         // transmit buffer
         int ct = UART_Send( &cmdBuff[ cmdNdx ], rspLen );
         rspLen -= ct;
         cmdNdx += ct;
         if( !rspLen )
            state = START_NEW_CMD;
         return;
      }

      case STATE_WAIT_BINARY:
      {
         int ch = UART_Recv();
         if( ch < 0 ) return;

         // If the previous character received was an escape character
         // then just save this byte (assuming there's space)
         if( flag & FLG_ESC )
         {
            flag &= ~FLG_ESC;
            if( cmdNdx < sizeof(cmdBuff) )
               cmdBuff[ cmdNdx++ ] = ch;
            return;
         }

         // If this is an escape character, don't save it
         // just keep track of the fact that we saw it.
         if( ch == ESC )
         {
            flag |= FLG_ESC;
            return;
         }

         // If this is an End Of Command character
         // then process the command
         if( ch == EOC )
         {
            rspLen = ProcessBinaryCmd( cmdBuff, cmdNdx, sizeof(cmdBuff) );
            cmdNdx = 0;
            state = STATE_SEND_BINARY_RSP;
            return;
         }

         // For other boring characters, just save them 
         // if there's space in my buffer
         if( cmdNdx < sizeof(cmdBuff) )
            cmdBuff[ cmdNdx++ ] = ch;
         return;
      }

      // Sending a response to a binary command
      case STATE_SEND_BINARY_RSP:

         // If there's no data left to send, then just send the
         // EOC character and start waiting for the next command
         if( !rspLen )
         {
            if( UART_SendByte( EOC ) )
               state = START_NEW_CMD;
            return;
         }

         // See what the next character to send is.
         int ch = cmdBuff[ cmdNdx ];

         // If it's a special character, I need to escape it.
         // I'll only do that if there are at least two spots 
         // in the transmit buffer to keep things simple here
         if( (ch == ESC) || (ch == EOC) )
         {
            if( UART_TxFree() >= 2 )
            {
               UART_SendByte( ESC );
               UART_SendByte( ch );
               cmdNdx++;
               rspLen--;
            }
            return;
         } 

         // Otherwise, just send the byte
         if( UART_SendByte( ch ) )
         {
            rspLen--;
            cmdNdx++;
         }
         return;
   }
}

