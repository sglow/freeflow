/* adc.c */

#include "adc.h"
#include "cpu.h"
#include "main.h"
#include "store.h"
#include "timer.h"
#include "trace.h"
#include "utils.h"
#include "vars.h"

// local data
static uint16_t batVolt;
static VarInfo varVbat;

void AdcInit( void )
{
   ADC_Regs *adc = (ADC_Regs *)ADC_BASE;

   // Configure the two pins that I'm using as analog inputs
   // PA2 (ADC12_IN7) (input voltage)
   GPIO_PinMode( DIGIO_A_BASE, 2, GPIO_MODE_ANALOG );

   // Perform a power-up and calibration sequence on both
   // A/D converters

   // Exit deep power down mode and turn on the
   // internal voltage regulator.
   adc->adc[0].ctrl   = 0x10000000;

   // Wait for the startup time specified in the datasheet
   // for the voltage regulator to become ready.
   // The time in the datasheet is 20 microseconds, but
   // I'll wait for 30 just to be extra conservative
   // (Hey, it's only 10 more microseconds!)
   BusyWait( 30 );

   // Calibrate the A/D for single ended channels
   adc->adc[0].ctrl |= 0x80000000;

   // Wait until the CAL bit is cleared meaning
   // calibration is finished
   while( adc->adc[0].ctrl & 0x80000000 ){}

   // Clear all the status bits
   adc->adc[0].stat = 0x3FF;

   // Enable the A/D
   adc->adc[0].ctrl |= 0x00000001;

   // Wait for the ADRDY status bit to be set
   while( !(adc->adc[0].stat & 0x00000001) ){}

   // Configure the A/D as 12-bit resolution
   adc->adc[0].cfg[0] = 0x00000000;

   // Setup oversampling mode to 256x oversample with a 4 bit right shift.
   // That should give me a 16 bit results
   adc->adc[0].cfg[1] = 0x0000009d;

   // Set sample time. I'm using 247.5 A/D clocks (about 3 usec)
   // to sample.  In my testing it doesn't really make very much
   // difference.
   adc->adc[0].samp[0] = 6;

   // Configure A/D 1 to sample channel 7 (vin)
   // and A/D 2 to sample channel 7 (vin)
   adc->adc[0].seq[0] = 7 << 6;

   VarInit( &varVbat, VARID_VIN, "bat_volt", VAR_TYPE_INT16, &batVolt, VAR_FLG_READONLY );
}

// Read the two A/D inputs 
// This is called from the high priority loop
void AdcRead( void )
{
   // TODO, for now I'm just manually starting
   ADC_Regs *adc = (ADC_Regs *)ADC_BASE;

   batVolt  = adc->adc[0].data;

   adc->adc[0].ctrl  |= 4;
}

float GetBatVolt( void )
{
   return batVolt;
}
