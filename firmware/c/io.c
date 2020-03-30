/* io.c */

#include "cpu.h"
#include "io.h"
#include "loop.h"

// This module controls simple I/O
// An error LED and two push buttons
//
// The LED output is connected to PB0
// The stand alone push button is on PC14
// The button on the encoder is on PC15

#define LED_STATE_OFF             0
#define LED_STATE_BLINK1          1
#define LED_STATE_BLINK0          2
#define LED_STATE_WAIT            3

static uint32_t blinkCt;
static uint32_t ledTime;
static uint8_t ledState;

void IntiIO( void )
{
   // Configure the three I/O pins
   GPIO_PinMode( DIGIO_B_BASE,  0, GPIO_MODE_OUTPUT );
   GPIO_PinMode( DIGIO_C_BASE, 14, GPIO_MODE_INPUT  );
   GPIO_PinMode( DIGIO_C_BASE, 15, GPIO_MODE_INPUT  );
}

// Return the state (0 or 1) of button 1
// That's the stand alone push button
// 0 means the button isn't being pressed
int GetButton1( void )
{
   return GPIO_GetPin( DIGIO_C_BASE, 14 );
}

// Return the state (0 or 1) of button 2
// That's the button on the encoder
// 0 means the button isn't being pressed
int GetButton2( void )
{
   // Note that this button is wired so it's high
   // when not pressed.  Hence the XOR 
   return GPIO_GetPin( DIGIO_C_BASE, 15 ) ^ 1;
}

// Return the state of both buttons.
// Button 1 is bit 0, button 2 is bit 1
int GetButtons( void )
{
   int ret = 0;
   if( GetButton1() ) ret |= 1;
   if( GetButton2() ) ret |= 2;
   return ret;
}

// Set the Error LED up to blink count
// times, then wait for 1 second.
// Further calls to this function will
// be ignore while the LED is busy
// blinking or waiting.
//
// Returns non-zero on success or
// zero if the LED is currently busy
int BlinkLED( uint32_t count )
{
   // I'll only accept a new blink code if the 
   // LED state is idle
   if( ledState != LED_STATE_OFF )
      return 0;

   blinkCt = count;
   ledTime = GetLoopCt();
   ledState = LED_STATE_BLINK1;
   return 1;
}

// Called repeatedly from the background process
// This handles blinking the error LED
void PollIO( void )
{
   switch( ledState )
   {
      // Idle state, LED is off
      case LED_STATE_OFF:
         GPIO_ClrPin( DIGIO_B_BASE, 0 );
         break;

      // Blinking with LED on 
      case LED_STATE_BLINK1:
         GPIO_SetPin( DIGIO_B_BASE, 0 );
         if( LoopsSince( ledTime ) > MsToLoop(100) )
         {
            ledState = LED_STATE_BLINK0;
            ledTime = GetLoopCt();
         }
         break;

      // Blinking with LED off 
      case LED_STATE_BLINK0:
         GPIO_ClrPin( DIGIO_B_BASE, 0 );
         if( LoopsSince( ledTime ) > MsToLoop(200) )
         {
            ledTime = GetLoopCt();

            if( --blinkCt )
               ledState = LED_STATE_BLINK1;
            else
               ledState = LED_STATE_WAIT;
         }
         break;

      // Delay of 1 sec between blink codes
      case LED_STATE_WAIT:
         if( LoopsSince( ledTime ) > MsToLoop(1000) )
            ledState = LED_STATE_OFF;
         break;

      default:
         ledState = LED_STATE_OFF;
         break;
   }
}
