/* timer.c */

#include "cpu.h"
#include "timer.h"

// This module configures one of the general purpose timers to simply
// count up once / microsecond.  This timer can be used for short 
// delays of less then 65536 uSec (65 msec).
void TimerInit( void )
{
   // Just set the timer up to count every microsecond.
   TimerRegs *tmr = (TimerRegs *)TIMER16_BASE;
   tmr->reload = 0xffff;
   tmr->prescale = (CLOCK_RATE_MHZ-1);
   tmr->ctrl[0] = 1;
}

