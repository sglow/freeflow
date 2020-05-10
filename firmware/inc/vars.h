/* vars.h */

#ifndef _DEF_INC_VARS
#define _DEF_INC_VARS

#include <stdint.h>

// This structure defines a variable.
// It contains function pointers used to access the variables contents.
// vars.c has some standard function for normal variables, but these
// can be overridden with custom functions for certain variables if needed.
typedef struct _VarInfo
{
   const char *name;         // Variable name.  Used to identify the variable via ASCII commands
   uint16_t id;              // Variable ID.  Used to identify the variable via binary commands
   uint8_t size;             // Size of the variable data in bytes.
   uint8_t flags;            // Various info about variable
   void *ptr;                // Pointer to the variable data

   // This function is called by the binary serial 'get' command.
   // It gets the variable value and stores it in the passed buffer
   // which has at least max bytes of space available.
   //
   // The function returns an error code
   int (*get)( struct _VarInfo *info, uint8_t *buff, int max );

   // This function is called to set a variable via the binary
   // command.  The data to store to the variable is passed in the
   // buffer which has at least 'len' bytes of data in it.
   //
   // The function returns an error code
   int (*set)( struct _VarInfo *info, uint8_t *buff, int len );

} VarInfo;

// Variable types
#define VAR_TYPE_INT16          1
#define VAR_TYPE_INT32          2
#define VAR_TYPE_ARY16          3
#define VAR_TYPE_ARY32          4
#define VAR_TYPE_FLOAT          5

// Flags passed to VarInit
#define VAR_FLG_READONLY        0x01

// Variable IDs
#define VARID_TRACE_CTRL        0
#define VARID_TRACE_PERIOD      1
#define VARID_TRACE_SAMP        2
#define VARID_TRACE_VAR1        3
#define VARID_TRACE_VAR2        4
#define VARID_TRACE_VAR3        5
#define VARID_TRACE_VAR4        6
#define VARID_LOOP_FREQ         7
#define VARID_PRESSURE1         8
#define VARID_PRESSURE2         9
#define VARID_POFF1             10
#define VARID_POFF2             11
#define VARID_POFF_CALC         12
#define VARID_PCAL              13
#define VARID_VIN               14
#define VARID_FLOW              15

#define VARID_MAX               50

// prototypes
int VarInit( VarInfo *info, uint16_t id, const char *name, int type, void *ptr, uint8_t flags );
int HandleVarGet( uint8_t *cmd, int len, int max );
int HandleVarSet( uint8_t *cmd, int len, int max );
int VarGet16( VarInfo *info, uint8_t *buff, int max );
int VarGet32( VarInfo *info, uint8_t *buff, int max );
int VarSet16( VarInfo *info, uint8_t *buff, int len );
int VarSet32( VarInfo *info, uint8_t *buff, int len );

#endif
