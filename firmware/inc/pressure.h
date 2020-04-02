/* pressure.h */

#ifndef _DEF_INC_PRESSURE
#define _DEF_INC_PRESSURE

// prototypes
void InitPressure( void );
void PollPressure( void );
void SPI1_ISR( void );
uint16_t GetPressure1( void );
uint16_t GetPressure2( void );

#endif
