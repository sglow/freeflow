/* binary.h */

#ifndef _DEF_INC_BINARY
#define _DEF_INC_BINARY

#include <stdint.h>

// Minimum length of buffer passed to binary command handler
// Any command with a 'max' value lower then this will be
// ignored.
#define MIN_BUFFER_LEN    32

// Binary command codes
#define CMD_STATE             0
#define CMD_PEEK              1
#define CMD_POKE              2
#define CMD_GET               3
#define CMD_SET               4

// prototypes
int ProcessBinaryCmd( uint8_t *cmd, int ct, int max );

// Use this function to return a successful command response.
// It adds the 0 error code and checksum.
// len is the number of bytes of data with the command (not
// including the two byte header)
int AddCksum( uint8_t *cmd, int len );

// Use this function to return an error code
int ReturnErr( uint8_t *cmd, int err );


#endif
