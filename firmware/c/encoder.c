/* encoder.c */

#include "cpu.h"
#include "encoder.h"

// The board includes a rotary encoder which can be used
// to enter settings.
//
// The two encoder lines are on PA15 and PA1.
// Those two inputs can be tied to timer 2 channels 1 and 2.

void InitEncoder( void )
{
   // Configure the two pins for use by timer 2
   GPIO_PinAltFunc( DIGIO_A_BASE,  1, 1 );
   GPIO_PinAltFunc( DIGIO_A_BASE, 15, 1 );

   TimerRegs *tmr = (TimerRegs *)TIMER2_BASE;

   // Select encoder input mode for the timer
   tmr->slaveCtrl = 0x00000003;

   // Add filters to the clock inputs.
   tmr->ccMode[0] = 0x0000F1F1;

   // Set the reload register to the max value.
   // Pity to use the only 32-bit counter for this, but
   // it's just how the I/O worked out.
   tmr->reload    = 0xFFFFFFFF;

   // Enable the timer
   tmr->ctrl[0]   = 0x00000001;
}

// Get the current encoder count value
int GetEncoderCount( void )
{
   TimerRegs *tmr = (TimerRegs *)TIMER2_BASE;
   return tmr->counter;
}
