/* flash.h */

#ifndef _DEF_INC_FLASH
#define _DEF_INC_FLASH

#include <stdint.h>

// prototypes
int FlashErase( uint32_t addr );
int FlashWrite( uint32_t addr, uint32_t *data, int ct );

#endif
