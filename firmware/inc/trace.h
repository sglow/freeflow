/* trace.h */

#ifndef _DEF_INC_TRACE
#define _DEF_INC_TRACE

// prototypes
void TraceInit( void );
void SaveTrace( void );
void DbgTrace( uint16_t a, uint16_t b, uint16_t c );
void DbgTraceL( uint16_t a, uint32_t b );
static inline void DbgTraceP( uint16_t a, const void *b ){ DbgTraceL( a, (uint32_t)b ); }

#endif
