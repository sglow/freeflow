/* firmware.c */

#include "binary.h"
#include "cpu.h"
#include "errors.h"
#include "firmware.h"
#include "flash.h"
#include "string.h"
#include "utils.h"

#include "trace.h"

int HandleFwErase( uint8_t *cmd, int len, int max )
{
   int err = ERR_OK;
   for( uint32_t addr = MAIN_FW_START; addr<MAIN_FW_END; addr += FLASH_PAGE_LEN )
   {
      err = FlashErase( addr );
      if( err ) break;
   }

   return ReturnErr( cmd, err );
}

int HandleFwWrite( uint8_t *cmd, int len, int max )
{
   // Total command length must be at least 14.  That's
   // command byte, cksum byte, 4 address bytes and 
   // at least 8 data bytes to write
   if( len < 14 ) 
      return ReturnErr( cmd, ERR_MISSING_DATA );

   uint32_t addr = b2u32( &cmd[2] );

   // The address must be a multiple of 8 since flash is
   // programmed 8 bytes at a time
   if( addr & 7 )
      return ReturnErr( cmd, ERR_RANGE );

   // Find the number of bytes to write and make sure it's
   // a multiple of 8
   int ct = len-6;
   if( ct & 7 )
      return ReturnErr( cmd, ERR_RANGE );

   if( addr < MAIN_FW_START )
      return ReturnErr( cmd, ERR_RANGE );

   if( addr+ct >= MAIN_FW_END-8 )
      return ReturnErr( cmd, ERR_RANGE );


   // The data to write starts at cmd[6].  If that address 
   // is a multiple of 4 then I can treat the data as an array
   // of 32-bit words.  If not, I'll need to copy it to an 
   // address that is.
   uint32_t dAddr = (uint32_t)&cmd[6];
   if( dAddr & 3 )
   {
      dAddr &= ~3;
      memcpy( (void*)dAddr, &cmd[6], ct );
   }

   // Convert ct to a number of 32-bit words
   ct >>= 2;

   int err = FlashWrite( addr, (uint32_t *)dAddr, ct );

   return ReturnErr( cmd, err );
}

typedef struct
{
   uint32_t count;
   uint32_t crc;
} FlashEndData;

// Save the length of the program and it's CRC to flash
int HandleFwCRC( uint8_t *cmd, int len, int max )
{
   // Command length should be 10 bytes, that's the
   // two byte header plus four each for length and CRC
   if( len < 10 ) 
      return ReturnErr( cmd, ERR_MISSING_DATA );

   FlashEndData end;
   end.count = b2u32( &cmd[2] );
   end.crc   = b2u32( &cmd[6] );

   int err = FlashWrite( MAIN_FW_END-8, (uint32_t*)&end, 2 );

   return ReturnErr( cmd, err );
}

// Check the CRC of the main firmware.  This is called
// by the boot loader on startup
int CheckFwCRC( void )
{
   // The last two words of flash memory hold
   // the number of bytes of valid flash and
   // a 32-bit CRC
   FlashEndData *end = (FlashEndData *)(MAIN_FW_END-8);

   // Make sure the length is reasonable
   if( end->count >= MAIN_FW_END-MAIN_FW_START-8 )
      return 0;

   uint32_t tbl[256];
   for( int i=0; i<256; i++ )
   {
      uint32_t crc = i;

      for( int j=0; j<8; j++ )
      {
         if( crc & 1 )
            crc = (crc>>1) ^ 0xEDB88320;
         else
            crc >>= 1;
      }
      tbl[i] = crc;
   }

   uint8_t *fw = (uint8_t*)MAIN_FW_START;

   uint32_t crc = 0xffffffff;
   for( int i=0; i<end->count; i++ )
      crc = tbl[ 0xFF & (crc^fw[i]) ] ^ (crc>>8);
   crc ^= 0xffffffff;

   return end->crc == crc;
}

