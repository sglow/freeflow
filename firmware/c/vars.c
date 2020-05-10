/* vars.c */

#include "binary.h"
#include "errors.h"
#include "trace.h"
#include "utils.h"
#include "vars.h"

// Variables are the main way to read/write data from the sensor.

// prototypes
static int VarGetUnknown( VarInfo *info, uint8_t *buff, int len );
static int VarSetUnknown( VarInfo *info, uint8_t *buff, int len );
static int VarSetReadOnly( VarInfo *info, uint8_t *buff, int len );

// local data
static VarInfo *varList[ VARID_MAX ];

// Initialize a variable and populate the function pointers 
// based on variable type.
//   info - Pointer to the variable info structure to fill in
//   id   - Variable ID used to access it via binary interface
//   name - Variable name used to access it via ASCII interface
//   type - Variable type.  Determines which functions are used to 
//          access the variable by default
//   ptr  - Pointer to the data that the variable will access.
//
// Returns an error code or 0 on success
int VarInit( VarInfo *info, uint16_t id, const char *name, int type, void *ptr, uint8_t flags )
{
   if( id >= VARID_MAX )
      return ERR_RANGE;

   if( varList[id] )
      return ERR_ALREADY_DEFINED;

   varList[id] = info;

   info->name  = name;
   info->id    = id;
   info->ptr   = ptr;
   info->flags = flags;

   switch( type )
   {
      case VAR_TYPE_INT16:
         info->get = VarGet16;
         info->set = VarSet16;
         info->size = sizeof(uint16_t);
         break;

      case VAR_TYPE_INT32:
      case VAR_TYPE_FLOAT:
         info->get = VarGet32;
         info->set = VarSet32;
         info->size = sizeof(uint32_t);
         break;

      default:
         info->get = VarGetUnknown;
         info->set = VarSetUnknown;
         info->size = 0;
         break;
   }

   if( flags & VAR_FLG_READONLY )
      info->set = VarSetReadOnly;

   return ERR_OK;
}

// This is called when a binary get command is received
// The command will contain the following bytes:
//   <cmd>   - The command code for a get command
//   <cksum> - Checksum byte.  Already validated when this is called
//   <varid> - Variable ID (low byte)
//   <varid> - Variable ID (high byte)
//
// The response will be of the form:
//   <err>   - Error code
//   <cksum> - Checksum
//   <...>   - Variable data (1 byte minimum)
int HandleVarGet( uint8_t *cmd, int len, int max )
{
   // Get commands must have at least 4 bytes of data
   if( len < 4 )
      return ReturnErr( cmd, ERR_MISSING_DATA );

   uint16_t vid = b2u16( &cmd[2] );
   if( (vid >= VARID_MAX) || !(varList[vid]) )
      return ReturnErr( cmd, ERR_UNKNOWN_VAR );

   VarInfo *info = varList[vid];

   // Make sure the buffer is long enough to hold the
   // variable data and two byte header
   if( max < info->size+2 )
      return ReturnErr( cmd, ERR_SHORT_CMD );

   int err = info->get( info, &cmd[2], max-2 );
   if( err )
      return ReturnErr( cmd, err );

   return AddCksum( cmd, info->size );
}

// This is called when a binary set command is received
// The command will contain the following bytes:
//   <cmd>   - The command code for a get command
//   <cksum> - Checksum byte.  Already validated when this is called
//   <varid> - Variable ID (low byte)
//   <varid> - Variable ID (high byte)
//   <...>   - The remaining bytes are the variable data to set
int HandleVarSet( uint8_t *cmd, int len, int max )
{
   // Get commands must have at least 4 bytes of data
   if( len < 4 )
      return ReturnErr( cmd, ERR_MISSING_DATA );

   uint16_t vid = b2u16( &cmd[2] );
   if( (vid >= VARID_MAX) || !(varList[vid]) )
      return ReturnErr( cmd, ERR_UNKNOWN_VAR );

   VarInfo *info = varList[vid];

   // Make sure enough data was passed for this variable
   if( len < info->size+4 )
      return ReturnErr( cmd, ERR_MISSING_DATA );

   int err = info->set( info, &cmd[4], len-4 );
   return ReturnErr( cmd, err );
}

// Standard functions to get a 16 bit signed or unsigned variable
int VarGet16( VarInfo *info, uint8_t *buff, int max )
{
   // Make sure there's at least two bytes of space in the passed buffer
   if( max < sizeof(int16_t) )
      return ERR_MISSING_DATA;

   uint16_t val = *(uint16_t*)info->ptr;
   u16_2_u8( val, buff );
   return ERR_OK;
}

// Standard functions to get a 32 bit signed or unsigned variable
int VarGet32( VarInfo *info, uint8_t *buff, int max )
{
   // Make sure there's at least two bytes of space in the passed buffer
   if( max < sizeof(int32_t) )
      return ERR_MISSING_DATA;

   uint32_t val = *(uint32_t*)info->ptr;
   u32_2_u8( val, buff );
   return ERR_OK;
}

// Standard functions to set a 16 bit signed or unsigned variable
int VarSet16( VarInfo *info, uint8_t *buff, int len )
{
   // Make sure enough data was passed
   if( len < sizeof(int16_t) )
      return ERR_MISSING_DATA;

   uint16_t val = b2u16( buff );

   *(uint16_t*)info->ptr = val;
   return ERR_OK;
}

// Standard functions to set a 32 bit signed or unsigned variable
int VarSet32( VarInfo *info, uint8_t *buff, int len )
{
   // Make sure enough data was passed
   if( len < sizeof(int32_t) )
      return ERR_MISSING_DATA;

   uint32_t val = b2u32( buff );

   *(uint32_t*)info->ptr = val;
   return ERR_OK;
}

// Default functions if the type of variable passed to VarInit
// was not known.
static int VarGetUnknown( VarInfo *info, uint8_t *buff, int len ){ return ERR_UNKNOWN_TYPE; }
static int VarSetUnknown( VarInfo *info, uint8_t *buff, int len ){ return ERR_UNKNOWN_TYPE; }
static int VarSetReadOnly( VarInfo *info, uint8_t *buff, int len ){ return ERR_READ_ONLY; }
