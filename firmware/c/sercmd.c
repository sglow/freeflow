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
#include "usb.h"
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

void InitSerCmd( SerCmdInfo *info, int usb )
{
   info->usb    = usb;
   info->state  = 0;
   info->flag   = 0;
   info->cmdNdx = 0;
   info->rspLen = 0;
}

static inline int SendByte( SerCmdInfo *info, uint8_t ch )
{
   if( info->usb )
      return USB_SendByte( ch );
   else
      return UART_SendByte( ch );
}

static inline int SendData( SerCmdInfo *info, uint8_t *data, int len )
{
   if( info->usb )
      return USB_Send( data, len );
   else
      return UART_Send( data, len );
}

static inline int RecvByte( SerCmdInfo *info )
{
   if( info->usb )
      return USB_Recv();
   else
      return UART_Recv();
}

static inline int TxFree( SerCmdInfo *info )
{
   if( info->usb )
      return USB_TxFree();
   else
      return UART_TxFree();
}


// This function is constantly called from the background task.
// It reads bytes from the UART and detects the end of a command.
// When a new command has been received, it passes it to the command
// processor and then sends the response back out the UART
void PollSerCmd( SerCmdInfo *info )
{
   // NOTE - I'm planning to add support for an ASCII interface in the future, 
   // but for right now I'm just supporting the binary one.
   info->flag |= FLG_BINARY;

   switch( info->state )
   {
      // Start of a new command.
      // In ASCII mode we discard any white space at the start of a command.
      // In binary mode we just jump to the next state
      case START_NEW_CMD:
         info->cmdNdx = 0;
         if( info->flag & FLG_BINARY )
            info->state = STATE_WAIT_BINARY;
         else
         {
            // Read the next character.  This will be -1 if there
            // aren't any new characters available
            int ch = RecvByte( info );
            if( ch < 0 )
               return;

            // Discard white space
            if( strchr( " \n\r\t", ch ) )
               return;

            // For non white space, add to my command buffer
            // and move on to the next state
            info->buff[0] = ch;
            info->cmdNdx = 1;
            info->state = STATE_WAIT_ASCII;
         }
         return;

      // Reading bytes from the UART until the end of a command is reached.
      // ASCII mode
      case STATE_WAIT_ASCII:
      {
         int ch = RecvByte( info );
         if( ch < 0 ) return;

         if( strchr( "\n\r", ch ) )
         {
            info->buff[ info->cmdNdx ] = 0;

            info->rspLen = ProcessAsciiCmd( (char*)info->buff, sizeof(info->buff) );
            info->cmdNdx = 0;
            info->state = STATE_SEND_ASCII_RSP;
            return;
         }

         if( info->cmdNdx < sizeof(info->buff) )
            info->buff[ info->cmdNdx++ ] = ch;
         return;
      }

      // Sending a response to an ASCII command
      case STATE_SEND_ASCII_RSP:
      {
         // Add as many bytes as possible to the UART 
         // transmit buffer
         int ct = SendData( info, &info->buff[ info->cmdNdx ], info->rspLen );
         info->rspLen -= ct;
         info->cmdNdx += ct;
         if( !info->rspLen )
            info->state = START_NEW_CMD;
         return;
      }

      case STATE_WAIT_BINARY:
      {
         int ch = RecvByte( info );
         if( ch < 0 ) return;

         // If the previous character received was an escape character
         // then just save this byte (assuming there's space)
         if( info->flag & FLG_ESC )
         {
            info->flag &= ~FLG_ESC;
            if( info->cmdNdx < sizeof(info->buff) )
               info->buff[ info->cmdNdx++ ] = ch;
            return;
         }

         // If this is an escape character, don't save it
         // just keep track of the fact that we saw it.
         if( ch == ESC )
         {
            info->flag |= FLG_ESC;
            return;
         }

         // If this is an End Of Command character
         // then process the command
         if( ch == EOC )
         {
            info->rspLen = ProcessBinaryCmd( info->buff, info->cmdNdx, sizeof(info->buff) );
            info->cmdNdx = 0;
            info->state = STATE_SEND_BINARY_RSP;
            return;
         }

         // For other boring characters, just save them 
         // if there's space in my buffer
         if( info->cmdNdx < sizeof(info->buff) )
            info->buff[ info->cmdNdx++ ] = ch;
         return;
      }

      // Sending a response to a binary command
      case STATE_SEND_BINARY_RSP:

         while( info->rspLen )
         {
            // See what the next character to send is.
            int ch = info->buff[ info->cmdNdx ];

            // If it's a special character, I need to escape it.
            // I'll only do that if there are at least two spots 
            // in the transmit buffer to keep things simple here
            if( (ch == ESC) || (ch == EOC) )
            {
               if( TxFree( info ) < 2 )
                  return;
               SendByte( info, ESC );
               SendByte( info, ch );
               info->cmdNdx++;
               info->rspLen--;
            } 

            // Otherwise, just send the byte
            else if( SendByte( info, ch ) )
            {
               info->rspLen--;
               info->cmdNdx++;
            }
            else
               return;
         }

         // If there's no data left to send, then just send the
         // EOC character and start waiting for the next command
         if( SendByte( info, EOC ) )
            info->state = START_NEW_CMD;

         return;
   }
}

