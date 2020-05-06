/* firmware.h */

#ifndef _DEF_INC_FIRMWARE
#define _DEF_INC_FIRMWARE

#include <stdint.h>

// prototypes
int HandleFwErase( uint8_t *cmd, int len, int max );
int HandleFwWrite( uint8_t *cmd, int len, int max );
int HandleFwCRC( uint8_t *cmd, int len, int max );
int CheckFwCRC( void );

#endif
