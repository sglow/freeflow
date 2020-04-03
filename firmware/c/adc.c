/* adc.c */

#include "adc.h"
#include "cpu.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"
#include "vars.h"

// local data
static uint16_t pressure;
static uint16_t batVolt;

static uint16_t calData[9];
void AdcInit( void )
{
   ADC_Regs *adc = (ADC_Regs *)ADC_BASE;

   // Configure the two pins that I'm using as analog inputs
   // PA0 (ADC1_IN5 ) (differential pressure sensor)
   // PA2 (ADC12_IN7) (input voltage)
   // PA3 (ADC12_IN8) (opamp output)
   //
   // I can read the differential sensor either at IN5 or (if the opamp
   // is turned on) at IN8.  The opamp allows me to boost the gain if
   // I need to.
   //
   GPIO_PinMode( DIGIO_A_BASE, 0, GPIO_MODE_ANALOG );
   GPIO_PinMode( DIGIO_A_BASE, 2, GPIO_MODE_ANALOG );

   // Perform a power-up and calibration sequence on both
   // A/D converters

   // Exit deep power down mode and turn on the
   // internal voltage regulator.
   adc->adc[0].ctrl   = 0x10000000;
   adc->adc[1].ctrl   = 0x10000000;

   // Wait for the startup time specified in the datasheet
   // for the voltage regulator to become ready.
   // The time in the datasheet is 20 microseconds, but
   // I'll wait for 30 just to be extra conservative
   // (Hey, it's only 10 more microseconds!)
   BusyWait( 30 );

   // Calibrate the A/D for single ended channels
   adc->adc[0].ctrl |= 0x80000000;
   adc->adc[1].ctrl |= 0x80000000;

   // Wait until the CAL bit is cleared meaning
   // calibration is finished
   while( adc->adc[0].ctrl & 0x80000000 ){}
   while( adc->adc[1].ctrl & 0x80000000 ){}

   // Clear all the status bits
   adc->adc[0].stat = 0x3FF;
   adc->adc[1].stat = 0x3FF;

   // Enable the A/D
   adc->adc[0].ctrl |= 0x00000001;
   adc->adc[1].ctrl |= 0x00000001;

   // Wait for the ADRDY status bit to be set
   while( !(adc->adc[0].stat & 0x00000001) ){}
   while( !(adc->adc[1].stat & 0x00000001) ){}

   // Configure both A/Ds as 12-bit resolution
   adc->adc[0].cfg[0] = 0x00000000;
   adc->adc[1].cfg[0] = 0x00000000;

   // Setup oversampling mode (TBD)
   adc->adc[0].cfg[1] = 0x0000009d;
   adc->adc[1].cfg[1] = 0x00000000;

   // Set sample time. I'm using 247.5 A/D clocks (about 3 usec)
   // to sample.  In my testing it doesn't really make very much
   // difference.
   adc->adc[0].samp[0] = 6;
   adc->adc[1].samp[0] = 6;

   // Configure A/D 1 to sample channel 5 (pressure)
   // and A/D 2 to sample channel 7 (vin)
   adc->adc[0].seq[0] = 5 << 6;
   adc->adc[1].seq[0] = 7 << 6;

   // OPARANGE needs to be 1

   calData[0] = 14300; //    0 mL / sec
   calData[1] = 15532; //  200 mL / sec
   calData[2] = 18878; //  400 mL / sec
   calData[3] = 24290; //  600 mL / sec
   calData[4] = 32064; //  800 mL / sec
   calData[5] = 41230; // 1000 mL / sec
   calData[6] = 52639; // 1200 mL / sec
   calData[7] = 63640; // 1400 mL / sec
   calData[8] = 65535; // 1600 mL / sec
}

// Read the two A/D inputs 
// This is called from the high priority loop
void AdcRead( void )
{
   // TODO, for now I'm just manually starting
   ADC_Regs *adc = (ADC_Regs *)ADC_BASE;

   pressure = adc->adc[0].data;
   batVolt  = adc->adc[1].data;

   adc->adc[0].ctrl  |= 4;
   adc->adc[1].ctrl  |= 4;
}

uint16_t GetDiffPressure( void )
{
   return pressure;
}

uint16_t GetDPcal( void )
{
   if( pressure < calData[0] )
      return 0;

   for( int i=1; i<ARRAY_CT(calData); i++ )
   {
      if( pressure > calData[i] )
         continue;

      int N = pressure - calData[i-1];
      int D = calData[i] - calData[i-1];

      return (i-1)*200 + 200*N / D;
   }
   return 1600;
}

uint16_t GetBatVolt( void )
{
   return batVolt;
}

