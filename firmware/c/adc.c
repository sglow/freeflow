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
static uint16_t pressure;
static uint16_t batVolt;
static VarInfo varDPres;

void AdcInit( void )
{
   ADC_Regs *adc = (ADC_Regs *)ADC_BASE;

   // Configure the two pins that I'm using as analog inputs
   // PA0 (ADC1_IN5 ) (differential pressure sensor) - rev1 board only, now obsolete
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

   // Setup oversampling mode to 256x oversample with a 4 bit right shift.
   // That should give me a 16 bit results
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

   VarInit( &varDPres, VARID_DIFF_PRES, "diff_pres", VAR_TYPE_INT16, &pressure, VAR_FLG_READONLY );
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

float GetBatVolt( void )
{
   return batVolt;
}
