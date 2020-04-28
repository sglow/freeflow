/* calc.c */

#include "calc.h"
#include "cpu.h"
#include "pressure.h"
#include "utils.h"

// History of readings, used to display graphs.
#define MS_PER_HIST_SAMP    30
#define HIST_LEN            128  // Must be 2^n
static float presHist[ HIST_LEN ];
static float flowHist[ HIST_LEN ];
static uint8_t histNdx;
static uint8_t histCt;
static float presSum, flowSum;


void UpdateCalculations( void )
{
   // Every cycle I'll update my history info
   presSum += GetPressure1();
   flowSum += GetFlowRate();
   if( ++histCt >= MS_PER_HIST_SAMP )
   {
      histNdx = (histNdx+1) & (HIST_LEN-1);

      presHist[ histNdx ] = presSum / MS_PER_HIST_SAMP;
      flowHist[ histNdx ] = flowSum / MS_PER_HIST_SAMP;
      histCt = 0;
      presSum = 0;
      flowSum = 0;
   }
}

// Get current calculated tidal volume
float GetTV( void )
{
   return 123.45;
}

float GetPIP( void )
{
   return 0;
}

float GetPEEP( void )
{
   return 0;
}

// Get historic pressure data.
// ndx is how long in the past (0 = most recent, 1 is one sample ago, etc)
float GetPresHistory( uint8_t ndx )
{
   // I grab the history data with ints disabled because
   // this is called from the background loop and the history 
   // data is updated in the high priority ISR loop
   int p = IntSuspend();
   ndx = (histNdx-ndx) & (HIST_LEN-1);
   float ret = presHist[ndx]; 
   IntRestore(p);
   return ret;
}

float GetFlowHistory( uint8_t ndx )
{
   // I grab the history data with ints disabled because
   // this is called from the background loop and the history 
   // data is updated in the high priority ISR loop
   int p = IntSuspend();
   ndx = (histNdx-ndx) & (HIST_LEN-1);
   float ret = flowHist[ndx]; 
   IntRestore(p);
   return ret;
}

float GetPresAvg( uint16_t ms )
{
   int samp = (ms + MS_PER_HIST_SAMP/2) / MS_PER_HIST_SAMP;
   if( samp < 1 ) samp = 1;
   if( samp > HIST_LEN ) samp = HIST_LEN;

   float sum = 0;
   for( int i=0; i<samp; i++ )
   {
      int n = (histNdx-i) & (HIST_LEN-1);
      sum += presHist[n];
   }
   return sum / samp;
}

float GetFlowAvg( uint16_t ms )
{
   int samp = (ms + MS_PER_HIST_SAMP/2) / MS_PER_HIST_SAMP;
   if( samp < 1 ) samp = 1;
   if( samp > HIST_LEN ) samp = HIST_LEN;

   float sum = 0;
   for( int i=0; i<samp; i++ )
   {
      int n = (histNdx-i) & (HIST_LEN-1);
      sum += flowHist[n];
   }
   return sum / samp;
}
