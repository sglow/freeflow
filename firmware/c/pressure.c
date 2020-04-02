/* pressure.c */

#include "cpu.h"
#include "loop.h"
#include "pressure.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"

//#define REV2

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

// local functions
static void SelectSensor( int which );

// local data
static uint16_t isrLastRead;
static uint32_t pressure[2];
static uint8_t pressureState;
static uint32_t lastPressureRead;

void InitPressure( void )
{
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
}

uint16_t GetPressure1( void )
{
   return pressure[0]>>8;
}

uint16_t GetPressure2( void )
{
   return pressure[1]>>8;
}

// This is called from the high priority loop every cycle.
// It manages reading pressure data from the sensors
void PollPressure( void )
{
   uint32_t now = GetLoopCt();

   if( LoopsSince( lastPressureRead ) < MsToLoop(5) )
      return;

   lastPressureRead = now;

   SPI_Regs *spi = (SPI_Regs *)SPI1_BASE;

   SelectSensor( 1 );
//DbgTrace( 1, spi->status, 0 );

   spi->data = 0xAA00;
//   uint16_t a = spi->status;
   spi->data = 0x0000;
//   uint16_t b = spi->status;

//   DbgTrace( 2, a, b );

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
         pressure[0] = (((uint32_t)isrLastRead)<<16) | value;
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
         pressure[1] = (((uint32_t)isrLastRead)<<16) | value;
         pressureState = STATE_IDLE;
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
