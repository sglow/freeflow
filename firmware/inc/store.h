/* store.h */

#ifndef _DEF_INC_STORE
#define _DEF_INC_STORE

#include <stdint.h>
#include <utils.h>

#define CAL_POINTS         20

// This structure layout of the non-volatile parameter info
// stored in flash.
// The structure must be exactly 512 bytes long.
typedef struct
{
   uint32_t crc;            // 32-bit CRC of remaining structure
   uint8_t  count;          // Incremented on each write. 
   uint8_t  mark;           // Used to corrupt old blocks
   uint16_t info;           // Bit-mapped info about block.  Currently just 0 (for future use)

   uint32_t pOff[2];            // Pressure sensor offsets
   float    pcal[CAL_POINTS];   // Pressure difference calibration
   uint32_t rsvd[40];           // Reserved for future use.
} StoreData;

// prototypes
void StoreInit( void );
const StoreData *FindStore( void );
int StoreUpdtOff( uint32_t offset, const void *value, uint8_t len );

#define StoreUpdt( member, value, len )      StoreUpdtOff( offsetof( StoreData, member ), value, len )

#define StoreUpdt32( member, value )         StoreUpdtOff( offsetof( StoreData, member ), value, sizeof(uint32_t) )

#endif

