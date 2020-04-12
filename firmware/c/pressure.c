/* pressure.c */

#include "cpu.h"
#include "errors.h"
#include "loop.h"
#include "pressure.h"
#include "store.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"
#include "vars.h"

#define REV2

// The Gauge pressure sensors use an SPI serial interface
// All sensors share the same SPI bus with different chip selects:
//
// PA5 - SS for sensor 1
// PA0 - SS for sensor 2
// PA6 - MISO  
// PB5 - MOSI
// PB3 - CLK

// States for the local state machine
#define STATE_IDLE         0     // Idle, in between readings
#define STATE_READ1H       1     // Waiting to receive high byte from sensor 1
#define STATE_READ1L       2     // Waiting for low byte from sensor 1
#define STATE_READ2H       3     // Waiting for high byte from sensor 2
#define STATE_READ2L       4     // Waiting for low byte from sensor 2

// Various local flags
#define FLG_NEW_READING    0x0001
#define FLG_SAVE_POFF      0x0002

#define CAL_POINTS         20

// local functions
static void SelectSensor( int which );
static int SetOffsetTime( VarInfo *info, uint8_t *buff, int len );
static int SetCalData( VarInfo *info, uint8_t *buff, int len );

// local data
static uint16_t isrLastRead;
static uint32_t praw[2];
static int32_t  padj[2];
static uint8_t pressureState;
static uint32_t lastPressureRead;
static VarInfo varPressure[2];
static VarInfo varPoffset[2];
static VarInfo varOffCalc;
static VarInfo varPresCal;
static uint32_t pOff[2];
static uint32_t offSum[2];
static uint32_t flags;
static uint16_t offCalcTime;
static uint16_t offCalcCount;
static int32_t calData[CAL_POINTS];

void InitPressure( void )
{
   // Init some variables
   VarInit( &varPressure[0], VARID_PRESSURE1,   "pressure1", VAR_TYPE_INT32, &padj[0], VAR_FLG_READONLY );
   VarInit( &varPressure[1], VARID_PRESSURE2,   "pressure2", VAR_TYPE_INT32, &padj[1], VAR_FLG_READONLY );
   VarInit( &varPoffset[0],  VARID_POFF1,       "poff1",     VAR_TYPE_INT32, &pOff[0], 0 );
   VarInit( &varPoffset[1],  VARID_POFF2,       "poff2",     VAR_TYPE_INT32, &pOff[1], 0 );
   VarInit( &varOffCalc,     VARID_POFF_CALC,   "poffcalc",  VAR_TYPE_INT16, &offCalcTime, 0 );
   VarInit( &varPresCal,     VARID_PCAL,        "prescal",   VAR_TYPE_ARY32, &calData, 0 );

   varOffCalc.set = SetOffsetTime;
   varPresCal.set = SetCalData;

   // Configure the pins for SPI use
   GPIO_PinAltFunc( DIGIO_A_BASE, 6, 5 );
   GPIO_PinAltFunc( DIGIO_B_BASE, 3, 5 );
   GPIO_PinAltFunc( DIGIO_B_BASE, 5, 5 );

#ifdef REV2
   GPIO_Output( DIGIO_A_BASE, 0, 0 );
#endif
   GPIO_Output( DIGIO_A_BASE, 5, 0 );

   // SPI mode 0 for clock and phase
   // Max SCLK frequency 800 kHz
   // MSB first
   //
   // Configure the SPI module
   SPI_Regs *spi = (SPI_Regs *)SPI1_BASE;

   // Configure the SPI to work in 16-bit data mode
   // Enable RXNE interrupts
   spi->ctrl[1] = 0x0F40;

   // Configure for master mode, CPOL and CPHA both 0.  
   // Baud rate is Pclk / 128 = 80Mhz/128 = 625kHz.
   // The sensor has a max clock rate of 800kHz.
   spi->ctrl[0] = 0x0374;

   EnableInterrupt( INT_VECT_SPI1, 3 );

   for( int i=0; i<2; i++ )
      pOff[i] = FindStore()->pOff[i];

   for( int i=0; i<CAL_POINTS; i++ )
      calData[i] = FindStore()->pcal[i];
}

int16_t GetPressure1( void )
{
   return padj[0]>>8;
}

int16_t GetPressure2( void )
{
   return padj[1]>>8;
}

int16_t GetPressureDiff16( void )
{
   return Clip16( (padj[1]-padj[0])>>8 );
}

float GetPressurePSI( void )
{
   // I believe the pressure sensor range is 0 to 1 PSI over 10% to 90%
   // of the count range (2^24).  That is, 10% of 2^24 would be 0 psi and
   // 90% of 2^24 would be 1PSI.
   //
   // I zero adjust the readings, so to get PSI I just scale the adjusted
   // value by 80% of 2^24.
   return padj[0] / (0x01000000*.8);
}

/*
static const int32_t calData[] =
{
     11834,           // 100
     44079,           // 200
     95932,           // 300
    168308,           // 400
    261033,           // 500
    374397,           // 600
    511040,           // 700
    669814,           // 800
    840659,           // 900
   1029393,           // 1000
   1270340,           // 1100
   1518120,           // 1200
   1770262,           // 1300
   2072881,           // 1400
   2389831,           // 1500
};
*/

// Return calibrated flow rate in cc/sec units
float GetPressureFlowRate( void )
{
   float dp = padj[1]-padj[0];

   int prev = 0;
   for( int i=0; i<ARRAY_CT(calData); i++ )
   {
      if( dp <= calData[i] )
      {
         float N = dp - prev;
         float D = calData[i] - prev;

         return 100*(i) + 100*N/D;
      }
      prev = calData[i];
   }

   return 100 * ARRAY_CT(calData);
}

int16_t TracePressureFlowRate( void )
{
   return GetPressureFlowRate();
}

// This is called by the low priority background task
void BkgPollPressure( void )
{
   if( flags & FLG_SAVE_POFF )
   {
      BitClr( FLG_SAVE_POFF, &flags );
      StoreUpdt( pOff, pOff, sizeof(pOff) );
   }
}

// This is called from the high priority loop every cycle.
// It manages reading pressure data from the sensors
void LoopPollPressure( void )
{
   if( flags & FLG_NEW_READING )
   {
      BitClr( FLG_NEW_READING, &flags );

      if( offCalcTime )
      {
         offSum[0] += praw[0];
         offSum[1] += praw[1];
         offCalcCount++;
         offCalcTime--;
         if( !offCalcTime )
         {
            pOff[0] = offSum[0] / offCalcCount;
            pOff[1] = offSum[1] / offCalcCount;
            BitSet( FLG_SAVE_POFF, &flags );
         }
      }
   }

   uint32_t now = GetLoopCt();
   if( LoopsSince( lastPressureRead ) < MsToLoop(5) )
      return;

   lastPressureRead = now;

   SPI_Regs *spi = (SPI_Regs *)SPI1_BASE;

   SelectSensor( 1 );

   spi->data = 0xAA00;
   spi->data = 0x0000;

   pressureState = STATE_READ1H;
}

// Interrupt generated when a new 16-bit word is received
void SPI1_ISR( void )
{
   SPI_Regs *spi = (SPI_Regs *)SPI1_BASE;

   uint16_t value = spi->data;

   switch( pressureState )
   {
      case STATE_READ1H:
         isrLastRead = value;
         pressureState = STATE_READ1L;
         return;

      case STATE_READ1L:
         praw[0] = 0x00FFFFFF & ((((uint32_t)isrLastRead)<<16) | value);
         padj[0] = praw[0] - pOff[0];
#ifdef REV2
         SelectSensor( 2 );
         pressureState = STATE_READ2H;
         spi->data = 0xAA00;
         spi->data = 0x0000;
#else
         SelectSensor( 0 );
         pressureState = STATE_IDLE;
#endif
         return;

      case STATE_READ2H:
         isrLastRead = value;
         pressureState = STATE_READ2L;
         return;

      case STATE_READ2L:
         SelectSensor( 0 );
         praw[1] = 0x00FFFFFF & ((((uint32_t)isrLastRead)<<16) | value);
         padj[1] = praw[1] - pOff[1];
         pressureState = STATE_IDLE;
         flags |= FLG_NEW_READING;
         return;
   }
}

// Lower the slave select line for the specified sensor
//  0 - neither
//  1 - Sensor number 1
//  2 - Sensor number 2
static void SelectSensor( int which )
{
   GPIO_SetPin( DIGIO_A_BASE, 0 );
   GPIO_SetPin( DIGIO_A_BASE, 5 );

   if( which & 1 )
      GPIO_ClrPin( DIGIO_A_BASE, 5 );

   if( which & 2 )
      GPIO_ClrPin( DIGIO_A_BASE, 0 );

   // I need to wait at least 3us before
   // sending clocks.  I'll just busy wait here.
   if( which )
      BusyWait( 3 );
}

static int SetOffsetTime( VarInfo *info, uint8_t *buff, int len )
{
   int err = VarSet16( info, buff, len );
   if( err ) return err;

   IntDisable();
   offSum[0] = 0;
   offSum[1] = 0;
   offCalcCount = 0;
   IntEnable();

   return 0;
}

// Set the calibration data for the pressure difference.
// I expect 20 x 32-bit integers, so 80 bytes of data
static int SetCalData( VarInfo *info, uint8_t *buff, int len )
{
   if( len < 4*CAL_POINTS )
      return ERR_MISSING_DATA;

   for( int i=0; i<CAL_POINTS; i++ )
      calData[i] = b2u32( &buff[4*i] );

   StoreUpdt( pcal, calData, 4*CAL_POINTS );

   return 0;
}

