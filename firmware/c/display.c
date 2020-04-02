/* display.c */

#include "cpu.h"
#include "display.h"
#include "font.h"
#include "string.h"
#include "trace.h"
#include "utils.h"

// The display is 64 pixels by 128 pixels.
// Rows are grouped into 8 'pages' of 8 rows each.
// Each byte of RAM on the display has one pixel / row
// Ex, the first byte contains the first pixel in each of the first 8 rows
// The LSB is the top row
#define NUM_COLS        128
#define NUM_ROWS        64
#define NUM_PAGES       (NUM_ROWS/8)

#define DISP_ADDR       0x3C

// local functions
static int SetupDisplay( void );
static int SendI2cCmdByte( uint8_t cmd );
static int SendI2c( uint8_t *data, uint8_t len );

// local data
static uint8_t initDone;
static uint8_t dispBuff[ NUM_PAGES ][ NUM_COLS ];
static uint8_t dmaBuff[ NUM_COLS+1 ];

// Called at startup
void InitDisplay()
{
   // Pins PB6 and PB7 are the i2c interface to the OLED
   // display.  These can be routed to i2c1
   GPIO_PinAltFunc( DIGIO_B_BASE, 6, 4 );
   GPIO_PinAltFunc( DIGIO_B_BASE, 7, 4 );

   I2C_Regs *i2c = (I2C_Regs *)I2C1_BASE;

   // Configure the obserdly complex timing register to
   // the example values for a 16MHz input clock and 400kHz 
   // i2c clock
   i2c->timing  = 0x10320309;

   // Enable i2c module
   i2c->ctrl[0] = 0x00000001;

}

void UpdtDisplay( void )
{
   for( int p=0; p<NUM_PAGES; p++ )
   {
      SendI2cCmdByte( 0x00 );
      SendI2cCmdByte( 0x10 );
      SendI2cCmdByte( 0xB0 | (p&7) );

      dmaBuff[0] = 0x40;
      memcpy( &dmaBuff[1], &dispBuff[p][0], NUM_COLS );
      SendI2c( dmaBuff, NUM_COLS+1 );
   }
}

// Called from background task
void PollDisplay()
{
   if( !initDone )
   {
      initDone = SetupDisplay();
      return;
   }

   if( !dbgU8[0] )
      return;

   memset( dispBuff, 0, sizeof(dispBuff) );

   for( int i=0, x=0; i<dbgU8[0]; i++ )
   {
      uint8_t ch = dbgU8[1]+i;
      DrawChar( ch, x, i, &mono12p );
      uint8_t ndx = ch - mono12p.firstChar;
      x += mono12p.chars[ndx].xAdv;
   }

   UpdtDisplay();
   dbgU8[0] = 0;
}

// Draw a character into my display buffer.
// x & y give the pixel location of the upper left corner of the
// character
void DrawChar( uint8_t ch, int x, int y, const FontInfo *font )
{
   // Check to make sure the character value is in our font
   // If not we just don't print anything there
   if( (ch < font->firstChar) || (ch > font->lastChar) )
      return;

   // Convert ch to an index into our character array
   ch -= font->firstChar;

   const FontChar *fc = &font->chars[ch];

   // Make sure x & y are within my screen area
   if( (x<0) || (y<0) || (x>=(NUM_COLS-fc->xAdv)) || (y >= (NUM_ROWS-font->yAdv)) )
      return;

   // The xOff field of the character structure gives the number of 
   // columns of pixels of blank space to advance before starting
   // to draw the character.
   x += fc->xOff;

   // The actual pixel data that describes the character is stored 
   // in the bitmap array.  The bmOff field of the character gives
   // the offset into the fonts bitmap array.
   const uint8_t *bitmap = &font->bitmap[ fc->bmOff ];

   // Each byte of the bitmap represents up to 8 rows of a single column
   // of the font.  This allows us to quickly copy the bitmap data into
   // my display memory which is formatted similarly.
   // I can figure out how many bytes / column by dividing the fonts
   // yAdv (i.e. total height of all characters) by 8 and rounding up.
   // Find bytes / column
   int bpc = (font->yAdv+7)/8;

   // Find how many columns we're writing
   int cols = fc->bmLen / bpc;

   // Find the first page (group of 8 rows) that I'll
   // be writing to.
   int p1 = y>>3;

   // Find the number of pages I need to write to.
   // That's the number of bytes/col if we're evenly
   // aligned with a page, or bpc+1 if not.
   int pgCt = (y&7) ? bpc+1 : bpc;

   for( int c=0; c<cols; c++ )
   {
      // Combine all the bytes for this column into a single
      // large integer.  Note this restricts the max font height
      // to 23 bits.  I could change the type to uint64_t if I 
      // ever wanted to support larger fonts, but 23 bits is plenty
      // for this display
      uint32_t C = 0;
      for( int p=0; p<bpc; p++ )
      {
         C <<= 8;
         C |= *bitmap++;
      }

      // Shift the column down based on the lower 3 bits of 
      // my y value.  That ensures that I write to the correct
      // location in the buffer
      C <<= (y&7);

      for( int p=0; p<pgCt; p++ )
      {
         dispBuff[p1+p][c+x] |= C;
         C >>= 8;
      }
   }
}

// Try to initialize the display
// Returns 1 on success, 0 on failure
static int SetupDisplay( void )
{
   // Turn off the display
   int ok = SendI2cCmdByte( 0xAE );
   if( !ok ) return 0;

   // Set memory addressing to page mode
   uint8_t pageMode[] = {0, 0x20, 0x02};
   ok = SendI2c( pageMode, sizeof(pageMode) );
   if( !ok ) return 0;

   // Make sure RAM starts at the first row
   ok = SendI2cCmdByte( 0x40 );
   if( !ok ) return 0;

   // Set normal (not inverted) display
   ok = SendI2cCmdByte( 0xA6 );
   if( !ok ) return 0;

   // Reverse the display left/right
   ok = SendI2cCmdByte( 0xA0 );
   if( !ok ) return 0;

   // Enable the DC/DC converter
   uint8_t enableDCDC[] = { 0, 0x8D, 0x14 };
   ok = SendI2c( enableDCDC, sizeof(enableDCDC) );
   if( !ok ) return 0;

   // Turn the display on
   return SendI2cCmdByte( 0xAF );
}

static int SendI2cCmdByte( uint8_t cmd )
{
   uint8_t buff[2];
   buff[0] = 0;
   buff[1] = cmd;
   return SendI2c( buff, 2 );
}

static int SendI2c( uint8_t *data, uint8_t len )
{
   I2C_Regs *i2c = (I2C_Regs *)I2C1_BASE;

   i2c->intClr  = 0x00003F38;
   i2c->ctrl[1] = (DISP_ADDR<<1) | 0x02002000 | (len<<16);

   while( len )
   {
      // Check for NAK
      if( i2c->status & 0x10 )
      {
dbgLong[1]++;
         return 0;
      }

      // See if ready for next byte
      if( i2c->status & 0x02 )
      {
         i2c->txData = *data++;
         len--;
      }
   }

   return 1;
}
