/* loop.c */

#include "adc.h"
#include "cpu.h"
#include "loop.h"
#include "pressure.h"
#include "trace.h"
#include "utils.h"
#include "vars.h"

// This module holds the high priority main loop of the system.
// A timer is configured to generate an interrupt at some fixed
// frequency.  
//
// The interrupt handler for that interrupt will perform all the
// high priority tasks that need to be done in real time.
//
// The interrupt is configured to be a low priority interrupt,
// so other hardware interrupt handlers can interrupt it.

static uint32_t loopCt;
static int16_t loopFreq;
static VarInfo varLoopFreq;

void LoopInit( void )
{
   // Configure time 16 to generate an interrupt 
   // at the loop frequency
   TimerRegs *tmr = (TimerRegs *)TIMER15_BASE;

   // I'm assuming that the loop period will be some 
   // integer number of microseconds and much faster
   // then 65 ms.
   //
   // I set the timer up to count every microsecond
   // and generate an interrupt at the loop rate.
   int usPerLoop = 1000000 / LOOP_FREQ;

   tmr->reload   = usPerLoop-1;
   tmr->prescale = (CLOCK_RATE_MHZ-1);
   tmr->event    = 1;

   // Enable the interrupt with priority 15 (lowest priority interrupt)
   EnableInterrupt( INT_VECT_TMR15, 15 );

   loopFreq = LOOP_FREQ;
   VarInit( &varLoopFreq, VARID_LOOP_FREQ, "loop_freq", VAR_TYPE_INT16, &loopFreq, VAR_FLG_READONLY );
}

void LoopStart( void )
{
   TimerRegs *tmr = (TimerRegs *)TIMER15_BASE;
   tmr->status   = 0;
   tmr->intEna   = 0x00000001;
   tmr->ctrl[0]  = 1;
}

uint32_t GetLoopCt( void )
{
   return loopCt;
}

void LoopISR( void )
{
   // Clear the interrupt
   TimerRegs *tmr = (TimerRegs *)TIMER15_BASE;
   tmr->status = 0;

   loopCt++;

   AdcRead();
   PollPressure();

   SaveTrace();
}
