/* calc.h */

#ifndef _DEF_INC_CALC
#define _DEF_INC_CALC

#include <stdint.h>

// prototypes
void UpdateCalculations( void );
float GetTV( void );
float GetPIP( void );
float GetPEEP( void );
float GetPresHistory( uint8_t ndx );
float GetFlowHistory( uint8_t ndx );
float GetPresAvg( uint16_t ms );
float GetFlowAvg( uint16_t ms );


#endif
