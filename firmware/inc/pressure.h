/* pressure.h */

#ifndef _DEF_INC_PRESSURE
#define _DEF_INC_PRESSURE

// prototypes
void InitPressure( void );
void PollPressure( void );
void SPI1_ISR( void );
int16_t GetPressure1( void );
int16_t GetPressure2( void );
float GetPressureDiff( void );
int16_t GetPressureDiff16( void );
float GetPressureFlowRate( void );
int16_t TracePressureFlowRate( void );

#endif
