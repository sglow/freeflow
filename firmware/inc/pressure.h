/* pressure.h */

#ifndef _DEF_INC_PRESSURE
#define _DEF_INC_PRESSURE

#define PSI_TO_KPA            6.89475729
#define PRESSURE_PSI          0.145038
#define PRESSURE_CM_H2O       10.1972
#define PRESSURE_KPA          1

// prototypes
void InitPressure( void );
void LoopPollPressure( void );
void BkgPollPressure( void );
void SPI1_ISR( void );
float GetPressure1( void );
float GetPressure2( void );
float GetPressureDiff( void );
float GetFlowRate( void );


#endif
