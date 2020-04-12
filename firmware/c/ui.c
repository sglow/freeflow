/* ui.c */

#include "calc.h"
#include "display.h"
#include "encoder.h"
#include "io.h"
#include "loop.h"
#include "pressure.h"
#include "sprintf.h"
#include "timer.h"
#include "trace.h"
#include "ui.h"
#include "utils.h"

typedef void (*ScreenFunc)(void);

// local functions
static void SummaryScreen( void );
static void ShowPressureGraph( void );
static void ShowFlowGraph( void );
static void ShowDebug( void );

// List of screen functions.
static ScreenFunc screens[] =
{
   SummaryScreen,
   ShowPressureGraph,
   ShowFlowGraph,
   ShowDebug,
};

// local data
static uint32_t lastUpdt;

// Called once at startup
void InitUserInteface( void )
{
}

// Called by the background loop
void PollUserInteface( void )
{
   // I update the display every 50ms
   if( LoopsSince( lastUpdt ) < MsToLoop( 50 ) )
      return;

   lastUpdt = GetLoopCt();

   ClearDisplay();

   // I use the encoder to select which screen to display
   // Each detent of the encoder is 4 counts, so I drop the
   // bottom two counts.  
   int s = GetEncoderCount() >> 2;

   int tot = ARRAY_CT(screens);
   s = s % tot;
   if( s < 0 ) s += tot;

   // Call the screen specific function to add info to the
   // display.
   screens[s]();

   // Copy the local display info to the OLED 
   UpdateDisplay();
}

// Summary screen.  Default at startup.
// Shows a list of interesting parameters
static void SummaryScreen( void )
{
   char buff[80];

   SetFont( FONT_FREESANS_12 );

   // Find height of each row of text
   int dy = CrntFont()->yAdv;
   int y = 0;

   sprintf( buff, "Flow: % 3d ml/sec", (int)GetPressureFlowRate() );
   DrawString( buff, 0, y );

   y+= dy;
   sprintf( buff, "Pres: %4d cm", (int)(GetPressurePSI() * PRESSURE_CM_H2O) );
   DrawString( buff, 0, y );
}

static void ShowPressureGraph( void )
{
   SetFont( FONT_FREESANS_16 );

   int y = 0;

   char buff[80];
   sprintf( buff, "Pres: %4d cmH2O", (int)(GetPresAvg(200) * PRESSURE_CM_H2O) );
   DrawString( buff, 0, y );

   y += CrntFont()->yAdv + 4;

   int graphHeight = 64 - y;

   for( int i=0; i<128; i++ )
   {
      float p = GetPresHistory( i );
      if( p < 0 ) p = 0;
      if( p > 1 ) p = 1;

      p *= (float)graphHeight;

      SetPixel( i, 63 - p );
   }
}

static void ShowFlowGraph( void )
{
   SetFont( FONT_FREESANS_16 );

   int y = 0;

   char buff[80];
   sprintf( buff, "Flow: %3d ml/sec", (int)GetFlowAvg(200) );
   DrawString( buff, 0, y );

   y += CrntFont()->yAdv + 4;

   int graphHeight = 64 - y;

   for( int i=0; i<128; i++ )
   {
      float f = GetFlowHistory( i );
      if( f < 0 ) f = 0;
      if( f > 1000 ) f = 1000;

      f *= (float)graphHeight / 1000.0;

      SetPixel( i, 63 - f );
   }
}

static const char *dbgStr[4];
void AddDebugStr( const char *str )
{
   for( int i=2; i>=0; i-- )
      dbgStr[i+1] = dbgStr[i];
   dbgStr[0] = str;
}

static void ShowDebug( void )
{
   SetFont( FONT_FREESANS_12 );

   // Find height of each row of text
   int dy = CrntFont()->yAdv;
   int y = 0;

   for( int i=0; i<4; i++ )
   {
      if( dbgStr[i] )
         DrawString( dbgStr[i], 0, y );
      y += dy;
   }
}

