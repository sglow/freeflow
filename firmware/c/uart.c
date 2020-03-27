/* uart.c */

#include <string.h>
#include "cpu.h"
#include "uart.h"
#include "utils.h"

#define BAUDRATE     115200

// local data
static uint8_t rxBuff[128];
static uint8_t txBuff[128];
static uint8_t rxHead, rxTail;
static uint8_t txHead, txTail;

// Init UART 
int UART_Init( void )
{
   // Pins PA9 and PA10 are used as TX and RX for the UART.
   // Assign those pins to the UART1 module.
   GPIO_PinAltFunc( DIGIO_A_BASE,  9, 7 );
   GPIO_PinAltFunc( DIGIO_A_BASE, 10, 7 );

   UART_Regs *reg = (UART_Regs*)UART1_BASE;

   // Set baud rate register
   reg->baud = Clip16u( CLOCK_RATE / BAUDRATE );

   // Enable the UART and receive interrupts
   reg->ctrl[0] = 0x002D;
//reg->ctrl[0] = 0x000D;

   EnableInterrupt( INT_VECT_UART1, 3 );

   return 0;
}

// Add the byte the the transmit queue.
// Return 1 on success or 0 if the queue is full.
// This starts a transmit if the UART is currently not sending.
int UART_SendByte( uint8_t dat )
{
   UART_Regs *reg = (UART_Regs*)UART1_BASE;

   int ok = 1;
   int p = IntSuspend();

   // See if the transmit buffer is empty and transmit ints are disabled
   if( (txHead == txTail) && !(reg->ctrl[0] & 0x0080) )
   {
      // Send the byte and enable transmit interrupts
      reg->txDat = dat;
      reg->ctrl[0] |= 0x0080;
   }

   // Otherwise, add this to the transmit buffer
   else
   {
      uint8_t newHead = txHead + 1;
      if( newHead >= sizeof(txBuff) ) 
         newHead = 0;

      if( newHead == txTail )
         ok = 0;
      else
      {
         txBuff[ txHead ] = dat;
         txHead = newHead;
      }
   }

   IntRestore(p);
   return ok;
}

// Add the passed date to the transmit queue and start sending it.
// The number of bytes added to the queue is returned
int UART_Send( uint8_t dat[], int ct )
{
   for( int i=0; i<ct; i++ )
   {
      if( !UART_SendByte( dat[i] ) )
         return i;
   }
   return ct;
}

// Send the passed string 
// Returns the number of characters sent
int UART_SendStr( const char *str )
{
   int i;
   for( i=0; *str; i++ )
   {
      if( !UART_SendByte( (uint8_t)*str++ ) )
         break;
   }
   return i;
}

// Read the next byte from the receive buffer
// Return the byte value, or -1 if none are available
int UART_Recv( void )
{
   int ret;
   int p = IntSuspend();
   if( rxHead == rxTail )
      ret = -1;
   else
   {
      ret = rxBuff[ rxTail++ ];
      if( rxTail >= sizeof(rxBuff) )
         rxTail = 0;
   }
   IntRestore(p);

   return ret;
}

// Return the number of bytes in our receive buffer
int UART_RxFull( void )
{
   int p = IntSuspend();
   int h = rxHead;
   int t = rxTail;
   IntRestore(p);

   int ct = h-t;
   if( ct < 0 ) ct += sizeof(rxBuff);
   return ct;
}

// Return the number of free spaces in the transmit buffer
int UART_TxFree( void )
{
   int p = IntSuspend();
   int h = txHead;
   int t = txTail;
   IntRestore(p);

   int ct = h-t;
   if( ct < 0 ) ct += sizeof(txBuff);

   return sizeof(txBuff) - 1 - ct;
}

// Flush the receive buffer
int UART_FlushRx( void )
{
   int p = IntSuspend();
   rxHead = rxTail = 0;
   IntRestore(p);

   return 0;
}

void UART_ISR( void )
{
   UART_Regs *reg = (UART_Regs*)UART1_BASE;

//   if( reg->status & 0x0002 )
//   {
//      uint8_t dat = reg->data;
//   }

   // See if we received a new byte
   if( reg->status & 0x0020 )
   {
      uint8_t val = reg->rxDat;

      uint8_t newHead = (rxHead+1);
      if( newHead >= sizeof(rxBuff) )
         newHead = 0;

      if( newHead != rxTail )
      {
         rxBuff[ rxHead ] = val;
         rxHead = newHead;
      }
   }

   // Check for transmit data register empty
   if( reg->status & reg->ctrl[0] & 0x0080 )
   {
      // If there's nothing left in the transmit buffer, 
      // just disable further transmit interrupts.
      if( txHead == txTail )
         reg->ctrl[0] &= ~0x0080;
      else
      {
         // Send the next byte
         reg->txDat = txBuff[ txTail++ ];
         if( txTail >= sizeof(txBuff) )
            txTail = 0;
      }
   }
}
