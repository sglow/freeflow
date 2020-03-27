/* main.c */

#include "buzzer.h"
#include "cpu.h"
#include "sercmd.h"
#include "uart.h"
#include "utils.h"

int main( void )
{
   // I use the first 128 bytes of RAM as a scratch
   // pad area for debugging.  Clear that data out
   // initially.
   for( int i=0; i<32; i++ )
      dbgLong[i] = 0;

   // Init the processor and various modules
   CPU_Init();
   UART_Init();
   BuzzerInit();

GPIO_PinMode( DIGIO_B_BASE, 0, GPIO_MODE_OUTPUT );
GPIO_PinMode( DIGIO_B_BASE, 6, GPIO_MODE_OUTPUT );
GPIO_PinMode( DIGIO_B_BASE, 7, GPIO_MODE_OUTPUT );

   // The main loop handles lower priority background tasks
   // The higher priority work is done in interrupt handlers.
   while( 1 )
   {
      PollSerCmd();
      BuzzerPoll();
   }
}
