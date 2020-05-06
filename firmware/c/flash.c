/* flash.c */

#include "cpu.h"
#include "errors.h"
#include "flash.h"
#include "utils.h"

#include "trace.h"

#define FLASH_END       (FLASH_START+FLASH_SIZE)

// Erase the page of flash at this address
int FlashErase( uint32_t addr )
{
   // Make sure the address is the start of a flash page
   if( (addr < FLASH_START) || (addr >= FLASH_END) )
      return ERR_RANGE;

   if( addr & (FLASH_PAGE_LEN-1) )
      return ERR_RANGE;

   FlashReg *reg = (FlashReg *)FLASH_BASE;

   // Clear all the status bits
   reg->status = 0x0000C3FB;

   // Unlock flash
   reg->key = 0x45670123;
   reg->key = 0xCDEF89AB;

   // Find the page number
   int n = (addr-FLASH_START) / FLASH_PAGE_LEN;

   reg->ctrl = 0x00000002 | (n<<3);
   reg->ctrl |= 0x00010000;

   // Wait for the busy bit to clear
   while( reg->status & 0x00010000 ){}

   // Lock the flash again
   reg->ctrl = 0x80000000;
   return 0;
}

// Write ct 32-bit integers to flash at the given address.
// ct must be even since flash is programmed 64-bits at a time.
int FlashWrite( uint32_t addr, uint32_t *data, int ct )
{
   // Make sure the address is the start of a flash page
   if( (addr < FLASH_START) || ((addr+4*ct) > FLASH_END) )
      return ERR_RANGE;

   if( (ct < 0) || (ct&1) || (addr&7) )
      return ERR_RANGE;

   FlashReg *reg = (FlashReg *)FLASH_BASE;

   // Clear all the status bits
   reg->status = 0x0000C3FB;

   // Unlock flash
   reg->key = 0x45670123;
   reg->key = 0xCDEF89AB;

   // Set the PG bit to start programming
   reg->ctrl = 0x00000001;

   uint32_t *dest = (uint32_t*)addr;
   for( int i=0; i<ct/2; i++ )
   {
      // Write 64-bits
      *dest++ = *data++;
      *dest++ = *data++;

      // Wait for busy bit to clear
      while( reg->status & 0x00010000 ){}

      // Check fo rerrors
      if( reg->status & 0x0000C3FA )
         break;

      // Clear EOP
      reg->status = 0x00000001;
   }

   // Lock flash
   reg->ctrl = 0x80000000;

   if( reg->status & 0x0000C3FA )
      return ERR_FLASH;
   return 0;
}

