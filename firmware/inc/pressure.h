/* pressure.h */

#ifndef _DEF_INC_PRESSURE
#define _DEF_INC_PRESSURE

#define PRESSURE_PSI          1
#define PRESSURE_CM_H2O       70.307
#define PRESSURE_KPA          6.89476

// prototypes
void InitPressure( void );
void LoopPollPressure( void );
void BkgPollPressure( void );
void SPI1_ISR( void );
int16_t GetPressure1( void );
int16_t GetPressure2( void );
float GetPressureDiff( void );
int16_t GetPressureDiff16( void );
float GetPressureFlowRate( void );
int16_t TracePressureFlowRate( void );
float GetPressurePSI( void );

#endif
