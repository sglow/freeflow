/* trace.c */

#include <stdint.h>
#include "adc.h"
#include "errors.h"
#include "pressure.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"
#include "vars.h"

// Data trace module
// This module is primarily used for development / debug.
// The trace allows certain interesting variables (like pressure readings for example) to be
// saved to memory at a fixed rate.  Later, the user interface code can download those readings
// and graph them or do math on them or whatever.
//
// The trace is configured using a few system variable:
//
// trace_ctrl - 16-bit control/status register.  It's bitmapped as follows:
//   xxxxxxxxxxxxxxxx
//   ...............\------ Set if the trace is currently running.  Clears when the trace fills up
//   ..............\------- Set to put the trace buffer in a special debug mode.  See below
//   \\\\\\\\\\\\\\-------- Reserved.  Should be left 0
//
// trace_period - 16-bit unsigned value which gives the period of data samples in units of loop cycles
//
// trace_samples - Number of samples collected so far
//
// trace_var1 - ID of first variable to trace.  0 for none
// trace_var2 - ID of second variable to trace.
// trace_var3 - ID of third variable to trace.
// trace_var4 - ID of fourth variable to trace.
//
// To start a trace, set the period and variable IDs to sample then set the control variable
// to start it running.

// Control bits
#define CTRL_RUNNING        0x0001
#define CTRL_DEBUG_TRACE    0x0002
#define CTRL_RESERVED       0xFFFC

// local functions
static uint16_t GetDbg0( void ){ return dbgUInt[0]; }
static uint16_t GetDbg1( void ){ return dbgUInt[1]; }
static uint16_t GetDbg2( void ){ return dbgUInt[2]; }
static uint16_t GetDbg3( void ){ return dbgUInt[3]; }
static int SetCtrl( VarInfo *info, uint8_t *buff, int len );

// Trace data is located at a fixed memory location
#define TRACE_DATA_ADDR 0x20008000
#define TRACE_DATA_LEN  0x00002000

// local data
static uint16_t varID[4];
static uint16_t period;
static uint16_t samples;
static uint16_t pct;
static uint16_t ctrl;
static uint16_t dbgTraceTime;
static VarInfo varCtrl, varPeriod, varSamples;
static VarInfo varTraceVar[4];

// Each trace variable has a function associated with it.
// that function is called in the high priority loop to sample
// the trace variable when it's being traced.
// The array below associates these functions with the variable IDs
typedef uint16_t (*traceFunc)(void);
static traceFunc traceVarFunc[] =
{
   0,                       // 0 None - trace variable ID 0 means nothing is sampled
   GetDbg0,                 // 1 dbgInt[0] - These are fixed locations at the start of RAM
   GetDbg1,                 // 2 dbgInt[1]   which can be used for debugging purposes.
   GetDbg2,                 // 3 dbgInt[2]   Just add code to put the data of interest in these
   GetDbg3,                 // 4 dbgInt[3]   locations and they can be sampled in real time.
   GetDiffPressure,         // 5 Differential pressure reading
   GetBatVolt,              // 6 Battery voltage
   GetDPcal,                // 7 Calibrated differential pressure
   GetPressure1,            // 8 Gauge pressure sensor 1
   GetPressure2,            // 9 Gauge pressure sensor 2
};


// One time init
void TraceInit( void )
{
   // Init the variables used to control the trace
   VarInit( &varCtrl,        VARID_TRACE_CTRL,   "trace_ctrl",    VAR_TYPE_INT16, &ctrl,     0 );
   VarInit( &varPeriod,      VARID_TRACE_PERIOD, "trace_period",  VAR_TYPE_INT16, &period,   0 );
   VarInit( &varSamples,     VARID_TRACE_SAMP,   "trace_samples", VAR_TYPE_INT16, &samples,  VAR_FLG_READONLY );
   VarInit( &varTraceVar[0], VARID_TRACE_VAR1,   "trace_var1",    VAR_TYPE_INT16, &varID[0], 0 );
   VarInit( &varTraceVar[1], VARID_TRACE_VAR2,   "trace_var2",    VAR_TYPE_INT16, &varID[1], 0 );
   VarInit( &varTraceVar[2], VARID_TRACE_VAR3,   "trace_var3",    VAR_TYPE_INT16, &varID[2], 0 );
   VarInit( &varTraceVar[3], VARID_TRACE_VAR4,   "trace_var4",    VAR_TYPE_INT16, &varID[3], 0 );

   varCtrl.set = SetCtrl;
}

// This is called at the end of the high priority main loop.
// We save trace data here.
void SaveTrace( void )
{
   // Just quit if the trace isn't running
   if( !(ctrl & CTRL_RUNNING) )
      return;

   // If the period counter is < period, we're done
   if( ++pct < period )
      return;

   pct = 0;

   uint16_t *traceData = (uint16_t *)TRACE_DATA_ADDR;

   // Save our trace variables to the buffer
   for( int i=0; i<4; i++ )
   {
      // I quit when I see an invalid variable ID
      if( !varID[i] || varID[i] >= ARRAY_CT(traceVarFunc) )
         break;

      // At this point, the variable ID values should have been vetted so I assume they're 
      // all valid.  Call the function to get the value to save
      uint16_t value = traceVarFunc[ varID[i] ]();

      // Save this to my trace data array
      traceData[ samples++ ] = value;
   }

   // If we can't store at least 4 more words to the array, quit now
   if( samples > TRACE_DATA_LEN/sizeof(uint16_t) - 4 )
      ctrl &= ~CTRL_RUNNING;
}

// Function called when trace control variable is set
static int SetCtrl( VarInfo *info, uint8_t *buff, int len )
{
   // Call the standard set function, but have it save
   // the value to a temporary location
   uint16_t tmp;
   info->ptr = &tmp;

   int err = VarSet16( info, buff, len );
   info->ptr = &ctrl;

   if( err ) return err;

   // Prevent setting reserved bits
   if( tmp & CTRL_RESERVED )
      return ERR_RANGE;

   // We support a special debug mode in which the trace buffer 
   // is used to store debug info in real time rather then being
   // sampled normally.  Setting this control bit resets the trace
   // and puts it in this mode.  If this bit is set, the RUNNING
   // bit is automatically cleared
   if( tmp & CTRL_DEBUG_TRACE )
   {
      ctrl = CTRL_DEBUG_TRACE;
      samples = 0;
      dbgTraceTime = TimerGetUsec();
      return 0;
   }

   // If the trace is being started, reset the 
   // sample and period counters
   if( !(ctrl & CTRL_RUNNING) && (tmp & CTRL_RUNNING) )
   {
      samples = 0;
      pct = 0;
   }

   ctrl = tmp;
   return 0;
}

// When this is called, 4 16-bit values will be written to the trace buffer.
// The first is the change in time (in usec) since this was previously called
// The other three are the values passed in here
void DbgTrace( uint16_t a, uint16_t b, uint16_t c )
{
   if( !(ctrl & CTRL_DEBUG_TRACE) )
      return;

   uint16_t *traceData = (uint16_t *)TRACE_DATA_ADDR;
   uint16_t now = TimerGetUsec();

   traceData[ samples++ ] = now - dbgTraceTime;
   traceData[ samples++ ] = a;
   traceData[ samples++ ] = b;
   traceData[ samples++ ] = c;

   dbgTraceTime = now;

   if( samples >= TRACE_DATA_LEN/(4*sizeof(uint16_t)) )
   {
      ctrl = 0;
      return;
   }
}

void DbgTraceL( uint16_t a, uint32_t b )
{
   DbgTrace( a, b>>16, b );
}


