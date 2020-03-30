/* adc.h */

#ifndef _DEF_INC_ADC
#define _DEF_INC_ADC

#include <stdint.h>

// prototypes
void AdcInit( void );
void AdcRead( void );
uint16_t GetDiffPressure( void );
uint16_t GetBatVolt( void );
uint16_t GetDPcal( void );

#endif
