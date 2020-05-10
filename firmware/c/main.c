/* main.c */

#include "adc.h"
#include "autooffset.h"
#include "buzzer.h"
#include "cpu.h"
#include "display.h"
#include "encoder.h"
#include "firmware.h"
#include "io.h"
#include "loop.h"
#include "pressure.h"
#include "sercmd.h"
#include "sprintf.h"
#include "store.h"
#include "timer.h"
#include "trace.h"
#include "uart.h"
#include "ui.h"
#include "usb.h"
#include "utils.h"

#ifdef BOOT
static void JumpToMainFirmware( void );
#endif

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

#ifdef BOOT
   JumpToMainFirmware();
#endif

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
uint16_t lastSend = TimerGetUsec();
   while( 1 )
   {
      PollSerCmd( &cmd[0] );
//      PollSerCmd( &cmd[1] );
      BuzzerPoll();
      PollIO();
      PollUserInterface();
      BkgPollPressure();
      PollUSB();

      // This is a temporary hack to stream data out the USB serial port
      // when it's open.
if( UsecSince( lastSend ) > 10000 )
{
   lastSend = TimerGetUsec();
      char buff[80];
      int len = sprintf( buff, "SLM: %5.3f\tPRS: % %5.2f\n", F2I(GetFlowRate()), F2I(GetPressure1()*PRESSURE_CM_H2O) );
      if( USB_TxFree() >= len )
         USB_Send( (uint8_t*)buff, len );
}
   }
}

// The mini version of the flow sensor doesn't compile in some of the
// modules that are used in the full version.  I'll just add dummy functions
// here to prevent link errors
#if defined(MINI) || defined(BOOT)
void InitDisplay( void ){}
void InitEncoder( void ){}
void InitUserInterface( void ){}
void BuzzerInit( void ){}
void DispISR( void ){}
void PollUserInterface( void ){}
void BuzzerPoll( void ){}
#endif

// The boot loader drops even more modules
#ifdef BOOT

// The boot loader calls this just after starting up
// It checks the main firmware CRC and if valid jumps
// to that code.
static void JumpToMainFirmware( void )
{
   // Check to see if the main firmware jumped to the 
   // boot loader intentionally.  If so we won't jump back.
   if( CheckSwap() )
      return;

   if( !CheckFwCRC() )
      return;

   SwapMode();
   return;
}
#endif
