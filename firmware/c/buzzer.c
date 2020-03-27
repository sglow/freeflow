/* buzzer.c */

#include "buzzer.h"
#include "cpu.h"
#include "utils.h"

// We have a buzzer on board which can be used to sound an
// alarm in the case of an error.  This buzzer is driven
// through an output pin tied to timer 1 channel 1

// One time init function, called at startup
void BuzzerInit( void )
{
   // Configure port A8 to be a timer output
   GPIO_PinAltFunc( DIGIO_A_BASE, 8, 1 );

   // Configure the timer for PWM mode
   TimerRegs *tmr = (TimerRegs *)TIMER1_BASE;

   // Configure timer output 1 for PWM mode
   tmr->ccMode[0] = 0x00000068;

   // My buzzer wants a 4kHz waveform, so
   // configure the timer period accordingly
   tmr->reload  = CLOCK_RATE / 4000;

   // Set the PWM duty cycle to 50% for now
   tmr->compare[0] = tmr->reload/2;

   // Clear the compare output enable bit
   // to keep the buzzer off for now
   tmr->ccEnable = 0;

   // Set the main output enable
   tmr->deadTime = 0x0000C000;

   // Start the timer
   tmr->ctrl[0] = 0x00000081;
}

// Background polling task for buzzer
void BuzzerPoll( void )
{
   TimerRegs *tmr = (TimerRegs *)TIMER1_BASE;
   if( dbgInt[0] )
      tmr->ccEnable = 1;
   else
      tmr->ccEnable = 0;
}
