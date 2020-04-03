/* display.c */

#include "cpu.h"
#include "display.h"
#include "font_sans9p.h"
#include "string.h"
#include "timer.h"
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

// States used for communicating with display
#define STATE_IDLE           0
#define STATE_SET_PAGE_ADDR  1     // Sending a page address
#define STATE_WRITE_PAGE     2     // Writing a page of display memory
#define STATE_DOING_INIT     3     // Doing one time init at startup

// local functions
static int DrawChar( uint8_t ch, int x, int y, const FontInfo *font );
static int SetupDisplay( void );
static void StartDmaWrite( const uint8_t *buff, uint8_t len );
static void SetPageAddr( uint8_t page, uint8_t col );
static void SendPage( uint8_t page );

// local data
static uint8_t dirtyPages;
static uint8_t dmaPage;
static volatile uint8_t dispState;

// The display buffer is a local copy of the RAM stored in the display itself.
// I write to this buffer then use DMA to send it (via i2c) to the display.
// The reason that there is one extra byte in each page (NUM_COL+1, not NUM_COL)
// is for convenience when doing the DMA transfer.  The display i2c interface
// requires an extra byte (always 0x40) to be sent before the data to identify the
// transfer as a data transfer as opposed to a command transfer.  Having that 
// extra byte available in the display buffer makes life much easer when setting
// up the DMA transfer.  When I write data to the buffer, I just offset the column
// by 1 to account for this.
static uint8_t dispBuff[ NUM_PAGES ][ NUM_COLS+1 ];

// Called at startup
void InitDisplay()
{
   // Pins PB6 and PB7 are the i2c interface to the OLED
   // display.  These can be routed to i2c1
   GPIO_PinAltFunc( DIGIO_B_BASE, 6, 4 );
   GPIO_PinAltFunc( DIGIO_B_BASE, 7, 4 );

   I2C_Regs *i2c = (I2C_Regs *)I2C1_BASE;

   // Configure the absurdly complex timing register to
   // the example values for a 16MHz input clock and 400kHz 
   // i2c clock
   i2c->timing  = 0x10320309;

   // Enable i2c module
   i2c->ctrl[0] = 0x00004021;

   // I'm using DMA1 channel 6 to transmit data through this i2c 
   // module.  Configure the channel selection register to assign
   // this function to that DMA channel.
   DMA_Reg *dma = (DMA_Reg *)DMA1_BASE;
   dma->chanSel = 0x00300000;

   // Setup the DMA channel, but don't enable it yet
   //
   // Note that the manual numbers DMA channels starting with 1, 
   // but I'm using an array so the index starts at 0.  That's why
   // I'm using channel[5] below.
   dma->channel[5].config = 0x00000090;
   dma->channel[5].pAddr = (uint32_t)&i2c->txData;
   EnableInterrupt( INT_VECT_I2C1, 3 );

   SetupDisplay();
}

void UpdtDisplay( void )
{
   dirtyPages = 0xFF;
   SetPageAddr( 0, 0 );
}

// Called from background task
void PollDisplay()
{
   if( !dbgU8[0] )
      return;

   if( dbgU8[0] == 0xff )
   {
      SetupDisplay();
      dbgU8[0] = 0;
      return;
   }

   memset( dispBuff, 0, sizeof(dispBuff) );

   for( int i=0, x=0; i<dbgU8[0]; i++ )
   {
      uint8_t ch = dbgU8[1]+i;
      x += DrawChar( ch, x, i+20, &sans9p );
   }

   UpdtDisplay();
   dbgU8[0] = 0;
}

// Draw a character into my display buffer.
// x & y give the pixel location of the upper left corner of the
// character
// Returns x advance for this character
static int DrawChar( uint8_t ch, int x, int y, const FontInfo *font )
{
   // Check to make sure the character value is in our font
   // If not we just don't print anything there
   if( (ch < font->firstChar) || (ch > font->lastChar) )
      return 0;

   // Convert ch to an index into our character array
   ch -= font->firstChar;

   const FontChar *fc = &font->chars[ch];

   // Make sure x & y are within my screen area
   if( (x<0) || (y<0) || (x>=(NUM_COLS-fc->xAdv)) || (y >= (NUM_ROWS-font->yAdv)) )
      return 0;

   // The xOff field of the character structure gives the number of 
   // columns of pixels of blank space to advance before starting
   // to draw the character.  
   x += fc->xOff;

   // Add one more to the column to account for the extra byte at the start of
   // each row in my display buffer.  The extra byte is to make DMA transfers
   // easer to set up.
   x += 1;

   // The actual pixel data that describes the character is stored 
   // in the bitmap array.  The bmOff field of the character gives
   // the offset into the fonts bitmap array.
   const uint8_t *bitmap = &font->bitmap[ fc->bmOff ];

   // Each byte of the bitmap represents up to 8 rows of a single column
   // of the font.  This allows us to quickly copy the bitmap data into
   // my display memory which is formatted similarly.
   // I can figure out how many bytes / column by dividing the font's
   // yAdv (i.e. total height of all characters) by 8 and rounding up.
   // Find bytes / column
   int bpc = (font->yAdv+7)/8;

   // Find how many columns we're writing
   int cols = fc->bmLen / bpc;

   // Find the first page (group of 8 rows) that I'll
   // be writing to.  That's basically the upper bits of the y
   // value.
   int p1 = y>>3;

   // Find the number of pages I need to write to.
   // That's the number of bytes/col if we're evenly
   // aligned with a page, or bpc+1 if not since the first 
   // and last page will get part of the font.
   int pgCt = (y&7) ? bpc+1 : bpc;

   // Update all the columns that this font touches
   for( int c=0; c<cols; c++ )
   {
      // Combine all the bytes for this column into a single
      // large integer.  I'm currently using a 32-bit integer which 
      // restricts the max font height to 25 bits.  I could change 
      // the type to uint64_t if I ever wanted to support larger fonts, 
      // but 25 bits is plenty for this display which only has 64 rows.
      uint32_t C = 0;
      for( int p=0; p<bpc; p++ )
      {
         C <<= 8;
         C |= *bitmap++;
      }

      // Shift the column lefd (down) based on the lower 3 bits of 
      // my y value.  That ensures that I write to the correct
      // location in the buffer.
      C <<= (y&7);

      // Update all the pages of display memory that the font touches
      for( int p=0; p<pgCt; p++ )
      {
         dispBuff[p1+p][c+x] |= C;
         C >>= 8;
      }
   }

   // I return the x advance value for this character
   return fc->xAdv;
}

static int SetupDisplay( void )
{
   // Send a command string to initialize the display
   static const uint8_t dispInitCmd[] =
   {
      0x00,            // All command strings start with 0
      0xAE,            // Turn the display off
      0x20, 0x02,      // Set memory addressing to page mode
      0x40,            // Make sure RAM starts at the first row
      0xA6,            // Set normal (not inverted) display
      0xA0,            // Reverse the display left/right
      0x8D, 0x14,      // Enable the DC/DC converter
      0xAF             // Turn the display on
   };

   dispState = STATE_DOING_INIT;
   StartDmaWrite( dispInitCmd, sizeof(dispInitCmd) );

   // Wait up to 500 microseconds for init to finish.
   // Normally takes about 240
   uint16_t start = TimerGetUsec();
   while( (dispState != STATE_IDLE) && (UsecSince(start) < 500) ){}

   // TODO - I should really add some error handling.
   //        not sure exactly what to do though.
   UpdtDisplay();

   return 0;
}

// Start writing to the display using DMA
static void StartDmaWrite( const uint8_t *buff, uint8_t len )
{
   DMA_Reg *dma = (DMA_Reg *)DMA1_BASE;
   I2C_Regs *i2c = (I2C_Regs *)I2C1_BASE;

   // Setup the DMA.  It will handle writing each byte to the i2c module
   // when it's ready and generate an interrupt when finished
   dma->channel[5].mAddr = (uint32_t)buff;
   dma->channel[5].config &= ~1;
   dma->channel[5].count = len;
   dma->channel[5].config |= 1;

   // Configure the i2c module and send the address
   i2c->intClr  = 0x00003F38;
   i2c->ctrl[1] = (DISP_ADDR<<1) | 0x02002000 | (len<<16);
}

// Sets the address at which data will be written in the display
static void SetPageAddr( uint8_t page, uint8_t col )
{
   static uint8_t setAddrCmdBuff[4];
   setAddrCmdBuff[0] = 0x00;              // All commands start with zero
   setAddrCmdBuff[1] = 0xB0 | (page&7);   // Set page start address
   setAddrCmdBuff[2] = 0x00 | (col&0x0F); // Set lower column address
   setAddrCmdBuff[3] = 0x10 | (col>>4);   // Set upper column address

   dispState = STATE_SET_PAGE_ADDR;
   dmaPage = page;
   StartDmaWrite( setAddrCmdBuff, sizeof(setAddrCmdBuff) );
}

static void SendPage( uint8_t page )
{
   page &= 7;

   dirtyPages &= ~(1<<page);

   // The first byte that we send is always 0x40
   // which tells the display that this is data, 
   // not a command.  There's an extra column
   // in the display buffer for this purpose
   dispBuff[page][0] = 0x40;

   dispState = STATE_WRITE_PAGE;
   dmaPage = page;
   StartDmaWrite(dispBuff[page], NUM_COLS+1 );
}

void DispISR( void )
{
   // Clear the interrupt
   I2C_Regs *i2c = (I2C_Regs *)I2C1_BASE;
   i2c->intClr  = 0x00003F38;

   switch( dispState )
   {
      // We just finished sending a page address.
      // Now we should send that page's data
      case STATE_SET_PAGE_ADDR:
         SendPage( dmaPage );
         return;

      // We just finished sending a page of data
      // Start writing the next dirty page
      case STATE_WRITE_PAGE:
         while( ++dmaPage < 8 )
         {
            if( dirtyPages & (1<<dmaPage) )
            {
               SetPageAddr( dmaPage, 0 );
               return;
            }
         }
         dispState = STATE_IDLE;
         return;

      default:
         dispState = STATE_IDLE;
         return;
   }
}

