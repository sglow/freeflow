/* ui.c */

#include "display.h"
#include "encoder.h"
#include "io.h"
#include "loop.h"
#include "sprintf.h"
#include "timer.h"
#include "trace.h"
#include "ui.h"
#include "utils.h"

typedef void (*ScreenFunc)(void);

// local functions
static void SummaryScreen( void );
static void ShowTvGraph( void );

// List of screen functions.
static ScreenFunc screens[] =
{
   SummaryScreen,
   ShowTvGraph,
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

   sprintf( buff, "Tvol: %3d mL", 123 );
   DrawString( buff, 0, y );

   y+= dy;
   sprintf( buff, "PIP: %3d cmH2O", 25 );
   DrawString( buff, 0, y );

   y+= dy;
   sprintf( buff, "Peep: %3d cmH2O", 17 );
   DrawString( buff, 0, y );
}

static void ShowTvGraph( void )
{
   SetFont( FONT_FREESANS_16 );

   char buff[80];
   sprintf( buff, "Tvol: %3d mL", 123 );

   int y = 0;
   DrawString( buff, 0, y );

   y += CrntFont()->yAdv + 4;

   int graphHeight = 64 - y;

   int off = GetLoopCt() /10;

   for( int i=0; i<128; i++ )
   {
      int val = (i+off) & 127;
      if( val >= 64 )
         val = 128-val;

      val *= (float)graphHeight/64;

      SetPixel( i, val + y );
      DbgTrace( 1, i, val+y );
   }
}

