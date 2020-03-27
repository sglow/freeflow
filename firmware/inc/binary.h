/* binary.h */

#ifndef _DEF_INC_BINARY
#define _DEF_INC_BINARY

#include <stdint.h>

// Binary command codes
#define CMD_STATE             0
#define CMD_PEEK              1
#define CMD_POKE              2


// prototypes
int ProcessBinaryCmd( uint8_t *cmd, int ct, int max );


#endif
