/* binary.c */

#include "binary.h"
#include "errors.h"
#include "string.h"
#include "utils.h"

#define MIN_BUFFER_LEN    32

// Binary command handler
//
// Binary commands have the following format:
// <cmd> <cksum> <data>...
//
// The first byte is the command code.
// The second byte is a checksum byte.  
// Any additional bytes are data bytes specific to the command.
//
// The checksum is calculated by XORing all the bytes in the packet
// The result must be 0x55 for a valid packet.
//
// The response starts with an error code, followed by a cksum
// byte, followed by data.

// local functions
static uint8_t Cksum( uint8_t *cmd, int len );
static int AddCksum( uint8_t *cmd, int len );
static int ReturnErr( uint8_t *cmd, int err );
static int HandlePeek( uint8_t *cmd, int len, int max );
static int HandlePoke( uint8_t *cmd, int len, int max );


int ProcessBinaryCmd( uint8_t *cmd, int len, int max )
{
   // To keep error checking simpler here, we require a minimum 
   // buffer length for binary commands.  If this buffer is too 
   // small, just ignore the command
   if( max < MIN_BUFFER_LEN )
      return 0;

   int err = 0;

   if( len < 2 )
      err = ERR_SHORT_CMD;

   else if( Cksum( cmd, len ) != 0x55 )
      err = ERR_CKSUM;

   else
   {
      switch( cmd[0] )
      {
         // Indicate whether we're running in boot mode or normal mode
         case CMD_STATE:
            #ifdef BOOT
               cmd[2] = 1;
            #else
               cmd[2] = 0;
            #endif
            return AddCksum( cmd, 1 );

         case CMD_PEEK:
            return HandlePeek( cmd, len, max );

         case CMD_POKE:
            return HandlePoke( cmd, len, max );

         default:
            err = ERR_BAD_CMD;
            break;
      }
   }

   if( err )
      return ReturnErr( cmd, err );

   return 0;
}

static uint8_t Cksum( uint8_t *cmd, int len )
{
   uint8_t ret = 0;
   for( int i=0; i<len; i++ )
      ret ^= cmd[i];
   return ret;
}

// Add the error code and checksum for a command with len data bytes
// Returns the total command length (len+2)
static int AddCksum( uint8_t *cmd, int len )
{
   cmd[0] = ERR_OK;
   cmd[1] = Cksum( &cmd[2], len ) ^ 0x55;
   return len+2;
}

static int ReturnErr( uint8_t *cmd, int err )
{
   cmd[0] = err;
   cmd[1] = err ^ 0x55;
   return 2;
}

// Peek at memory locations.  Very handy for debugging
// Data passed is a 32-bit address and 8-bit number of bytes to return
static int HandlePeek( uint8_t *cmd, int len, int max )
{
   // Total command length must be at least 7.  That's
   // command byte, cksum byte, 4 address bytes and 1 count.
   if( len < 7 ) 
      return ReturnErr( cmd, ERR_MISSING_DATA );

   uint32_t addr = b2u32( &cmd[2] );
   int ct = cmd[6];

   // If the address is less then 0x80, then I add the offset to the
   // start of RAM.  This is just for convenience since I reserve the 
   // first few bytes of RAM for debugging purposes.
   if( addr < 0x80 ) addr += 0x20000000;

   // Limit the number of output bytes based on buffer size
   if( ct > max-2 )
      ct = max-2;

   // Copy data to my buffer
   memcpy( &cmd[2], (const void *)addr, ct );
   
   return AddCksum( cmd, ct );
}

// Poke to memory locations.  Very handy for debugging
// Data passed is a 32-bit address and data to write 
// starting there
static int HandlePoke( uint8_t *cmd, int len, int max )
{
   // Total command length must be at least 7.  That's
   // command byte, cksum byte, 4 address bytes and 
   // at least 1 data byte to write
   if( len < 7 ) 
      return ReturnErr( cmd, ERR_MISSING_DATA );

   uint32_t addr = b2u32( &cmd[2] );

   // If the address is less then 0x80, then I add the offset to the
   // start of RAM.  This is just for convenience since I reserve the 
   // first few bytes of RAM for debugging purposes.
   if( addr < 0x80 ) addr += 0x20000000;

   int ct = len-6;

   // If both the address and count are multiples of 4, I write
   // 32-bit values.  This is useful when poking into registers
   // that need to be written as longs.
   if( !(addr&3) && !(ct&3) )
   {
      ct /= 4;
      uint32_t *ptr = (uint32_t*)addr;
      for( int i=0; i<ct; i++ )
         *ptr++ = b2u32( &cmd[6+i*4] );
   }

   // Same idea for multiples of 2
   else if( !(addr&1) && !(ct&1) )
   {
      ct /= 2;
      uint16_t *ptr = (uint16_t*)addr;
      for( int i=0; i<ct; i++ )
         *ptr++ = b2u16( &cmd[6+i*2] );
   }

   else
   {
      uint8_t *ptr = (uint8_t*)addr;
      for( int i=0; i<ct; i++ )
         *ptr++ = cmd[6+i];
   }

   return ReturnErr( cmd, ERR_OK );
}
