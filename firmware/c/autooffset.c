/* autooffset.c */

// This module monitors the two pressure sensors and when they appear to 
// be quiet it slowly adjusts an offset value to try to remove drift from
// the flow readings.

#include "autooffset.h"
#include "filter.h"
#include "loop.h"
#include "math.h"
#include "pressure.h"
#include "utils.h"

// Maximum difference between the current pressure reading 
// and the filtered value to be considered moving
#define MAX_PRES_DIFF       0.005

#define GAIN                1e-5

// local data
static Filter presFilt[2];
static int ignoreCount;
static float autoOffset;

void InitAutoOffset( void )
{
   // 1 Hz 2-poll Butterworth 
   const float coefA[] = { -1.99111, 0.99115 };
   const float coefB[] = { 9.8259e-06, 1.9652e-05, 9.8259e-06 };
   for( int i=0; i<2; i++ )
      FilterInit( &presFilt[i], coefA, coefB );

   ignoreCount = LOOP_FREQ;
}

void LoopUpdtOffset( void )
{
   float p[2];

   p[0] = GetPressure1();
   p[1] = GetPressure2();

   // Run the pressure readings through a low pass filter.
   for( int i=0; i<2; i++ )
   {
      float f = FilterFlt( p[i], &presFilt[i] );

      if( fabsf( p[i]-f ) > MAX_PRES_DIFF )
         ignoreCount = LOOP_FREQ;
   }

   if( ignoreCount )
   {
      ignoreCount--;
      return;
   }

   // Get the difference in pressure which includes my
   // auto offset value.  This should be zero since we
   // believe there's no flow at the moment
   float pd = GetPressureDiff();

   autoOffset -= pd * GAIN;
}

float GetPresFilt1( void )
{
   return FilterOut( &presFilt[0] );
}

float GetPresFilt2( void )
{
   return FilterOut( &presFilt[1] );
}

float GetAutoOffset( void )
{
   return autoOffset;
}

void AutoOffsetClear( void )
{
   autoOffset = 0;
}
