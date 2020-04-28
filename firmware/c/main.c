/* main.c */

#include "adc.h"
#include "autooffset.h"
#include "buzzer.h"
#include "cpu.h"
#include "display.h"
#include "encoder.h"
#include "io.h"
#include "loop.h"
#include "pressure.h"
#include "sercmd.h"
#include "store.h"
#include "timer.h"
#include "trace.h"
#include "uart.h"
#include "ui.h"
#include "usb.h"
#include "utils.h"

int main( void )
{
   // I use the first 128 bytes of RAM as a scratch
   // pad area for debugging.  Clear that data out
   // initially.
   for( int i=0; i<32; i++ )
      dbgLong[i] = 0;

   SerCmdInfo cmd[2];

   // Init the processor and various modules
   CPU_Init();
   StoreInit();
   UART_Init();
   BuzzerInit();
   InitEncoder();
   IntiIO();
   TimerInit();
   LoopInit();
   AdcInit();
   TraceInit();
   InitPressure();
   InitDisplay();
   InitUserInterface();
   InitUSB();
   InitAutoOffset();
   InitSerCmd( &cmd[0], 0 );
//   InitSerCmd( &cmd[1], 1 );

   LoopStart();

   // The main loop handles lower priority background tasks
   // The higher priority work is done in interrupt handlers.
   while( 1 )
   {
      PollSerCmd( &cmd[0] );
//      PollSerCmd( &cmd[1] );
      BuzzerPoll();
      PollIO();
      PollUserInterface();
      BkgPollPressure();
      PollUSB();
   }
}

// The mini version of the flow sensor doesn't compile in some of the
// modules that are used in the full version.  I'll just add dummy functions
// here to prevent link errors
#ifdef MINI
void InitDisplay( void ){}
void InitEncoder( void ){}
void InitUserInterface( void ){}
void BuzzerInit( void ){}
void DispISR( void ){}
void PollUserInterface( void ){}
void BuzzerPoll( void ){}
#endif
