/* timer.h */

#ifndef _DEF_INC_TIMER
#define _DEF_INC_TIMER

#include <stdint.h>
#include "cpu.h"

// prototypes
void TimerInit( void );

static inline uint32_t TimerGetUsec( void )
{
   TimerRegs *tmr = (TimerRegs *)TIMER16_BASE;
   return tmr->counter;
}

static inline uint32_t UsecSince( uint16_t when )
{
   TimerRegs *tmr = (TimerRegs *)TIMER16_BASE;
   return tmr->counter - when;
}

#endif
