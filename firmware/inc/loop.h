/* loop.h */

#ifndef _DEF_INC_LOOP
#define _DEF_INC_LOOP

// Main loop frequency (Hz)
#define LOOP_FREQ      1000

// prototypes
void LoopInit( void );
void LoopStart( void );
void LoopISR( void );
uint32_t GetLoopCt( void );

static inline uint32_t LoopsSince( uint32_t when )
{
   return GetLoopCt() - when;
}

// Convert milliseconds to loop counts
static inline uint32_t MsToLoop( uint32_t ms )
{
   return ms * LOOP_FREQ/1000;
}

#endif
