/* errors.h */

#ifndef _DEF_INC_ERRORS
#define _DEF_INC_ERRORS

#define ERR_OK                0       // No error
#define ERR_CKSUM             1       // Invalid checksum on binary command
#define ERR_SHORT_CMD         2       // Command too short
#define ERR_BAD_CMD           3       // Invalid command code
#define ERR_MISSING_DATA      4       // Insufficiet data sent with command
#define ERR_UNKNOWN_TYPE      5       // Unknown variable type
#define ERR_RANGE             6       // Passed value is out of range
#define ERR_ALREADY_DEFINED   7       // Variable is already defined
#define ERR_UNKNOWN_VAR       8       // Variable isn't defined
#define ERR_READ_ONLY         9       // Variable is read only
#define ERR_FLASH             10      // Error erasing / programming flash
#define ERR_VERIFY            11      // Verify error checking storage block

#endif
