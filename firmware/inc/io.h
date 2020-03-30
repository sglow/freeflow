/* io.h */

#ifndef _DEF_INC_IO
#define _DEF_INC_IO

// prototypes
void IntiIO( void );
int GetButton1( void );
int GetButton2( void );
int GetButtons( void );
void PollIO( void );
int BlinkLED( uint32_t count );

#endif
