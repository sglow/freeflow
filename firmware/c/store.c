/* store.c */

#include "cpu.h"
#include "errors.h"
#include "flash.h"
#include "store.h"
#include "string.h"
#include "trace.h"

// This module handles non-volatile parameter storage 
//
// These parameters are stored in a structure of fixed size
// in two pages of flash memory.
//
// On system startup we look through these two pages of flash to find the
// valid parameters.  There should only be one valid copy under normal conditions
// If multiple copies are found (which could happen if we lose power during an 
// update for example), then the most recently written copy will be kept.
//
// When a parameter value is changed, the function StoreUpdt is called to 
// update the values in flash.  This function first writes a new copy of
// the parameter structure to an erased area of flash, and once that copy 
// is written and validated it overwrites part of the old copy to invalidate
// it.  In this way there should always be at least one valid copy of the
// parameters in flash memory and normally only one.

// Size of StoreData structure
#define STORE_DATA_SIZE          0x00000100

// Storage is in last two pages of flash
#define STORE_ADDR               (FLASH_START+FLASH_SIZE-2*FLASH_PAGE_LEN)

// Value of mark in valid block.  Pretty arbitrary, anything but 0x00 or 0xff
#define GOOD_MARK                0x55

// local functions
static int CheckStruct( uint32_t addr );
static uint32_t BlockCRC( void *blk );
static void Invalidate( uint32_t addr );
static int SaveBlock( StoreData *blk, uint32_t addr );

// local data
uint32_t storeAddr;

void StoreInit( void )
{
   // Make sure the size of the storage structure is correct.
   // This catches mistakes when adding new variables.
   // If it's wrong, then I call a function that doesn't actually
   // exist.  That generates a link time error, so the program
   // won't actually be built.
   // If the size is correct then the compiler will optimize this out.
   if( sizeof(StoreData) != STORE_DATA_SIZE )
   {
      extern void CheckSizeOfStoreData( void );
      CheckSizeOfStoreData();
   }

   // Total structures in two pages of flash
   int N = 2 * FLASH_PAGE_LEN / STORE_DATA_SIZE;

   // Run through structures looking for a valid one.
   uint32_t addr = STORE_ADDR;
   for( int i=0; i<N; i++, addr+=STORE_DATA_SIZE )
   {
      // Check the structure at this address
      if( !CheckStruct(addr) )
         continue;

      // If this is the first valid block we've found, 
      // just keep track of the address
      if( !storeAddr )
      {
         storeAddr = addr;
         continue;
      }

      // I've found multiple valid blocks.  This normally shouldn't
      // happen but could if power is lost at just the right time
      // when updating flash.
      //
      // I'll check the counter to see which was is newer and keep
      // that address.  I'll also invalidate the older one.
      StoreData *a = (StoreData *)storeAddr;
      StoreData *b = (StoreData *)addr;

      int8_t diff = b->count - a->count;
      if( diff > 0 )
      {
         Invalidate( storeAddr );
         storeAddr = addr;
      }
      else
         Invalidate( addr );
   }

   // If I found a valid parameter area, I'm done
   if( storeAddr )
      return;

   // If no valid storage was found then I'll just create one with 
   // zero values.
   StoreData blank;
   memset( &blank, 0, sizeof(blank) );
   blank.mark = GOOD_MARK;
   blank.crc = BlockCRC( &blank );

   FlashErase( STORE_ADDR );
   FlashWrite( STORE_ADDR, (uint32_t*)&blank, sizeof(blank)/sizeof(uint32_t) );
   storeAddr = STORE_ADDR;
}

const StoreData *FindStore( void )
{
   return (const StoreData*)storeAddr;
}

int StoreUpdtOff( uint32_t offset, const void *value, uint8_t len )
{
   // Make sure the passed pointer is pointing to somewhere
   // in the current page and isn't in the reserved first 8 bytes
   if( (offset < 8) || (offset >= STORE_DATA_SIZE) )
      return ERR_RANGE;

   // Copy the current parameter block to RAM
   StoreData temp;
   memcpy32( (uint32_t*)&temp, (uint32_t *)storeAddr, STORE_DATA_SIZE/4 );

   // Update the contents in RAM
   memcpy( (uint8_t*)&temp + offset, value, len );
   memset( &temp.rsvd, 0, sizeof(temp.rsvd) );

   temp.count++;
   temp.mark = GOOD_MARK;
   temp.crc = BlockCRC( &temp );

   uint32_t addr = storeAddr + STORE_DATA_SIZE;

   int err = SaveBlock( &temp, addr );

   // That shouldn't really fail, but if it does for some reason
   // try storing in the first location of the other flash page.
   // That will cause an erase first, so is more likely to succeed
   if( err )
   {
      addr = STORE_ADDR;
      if( storeAddr < STORE_ADDR + FLASH_PAGE_LEN )
         addr += FLASH_PAGE_LEN;
      err = SaveBlock( &temp, addr );
   }

   if( err ) return err;

   // After successfully storing and validating the new parameters,
   // Invalidate the old ones
   Invalidate( storeAddr );
   storeAddr = addr;

   return 0;
}

static int SaveBlock( StoreData *blk, uint32_t addr )
{
   // If the address is the start of a page, first erase the page
   if( (addr & (FLASH_PAGE_LEN-1)) == 0 )
   {
      int err = FlashErase( addr );
      if( err ) return err;
   }

   int err = FlashWrite( addr, (uint32_t*)blk, STORE_DATA_SIZE/4 );
   if( err ) return err;

   if( memcmp32( (uint32_t*)addr, (uint32_t *)blk, STORE_DATA_SIZE/4 ) )
      return ERR_VERIFY;

   return 0;
}

// Check the block at this address o see if it's valid.
// Returns 1 if it is, 0 if not
static int CheckStruct( uint32_t addr )
{
   StoreData *store = (StoreData *)addr;

   // We set the mark to a fixed value if it's good. 
   // That allows me to quickly identify bad blocks.
   // If the block is erased, the mark will be 0xFF
   // When I invalidate a block I set it to 0x00.
   // The good mark could be any other value picked
   // pretty much arbitrarily.
   if( store->mark != GOOD_MARK )
      return 0;

   uint32_t crc = BlockCRC( store );
   return store->crc == crc;
}

// Calculate the CRC of the block at this address
// This chip happens to have a hardware CRC generator, so I'll use that.
static uint32_t BlockCRC( void *blk )
{
   CRC_Regs *crc = (CRC_Regs *)crc;

   // Reset the CRC unit
   crc->ctrl = 1;

   uint32_t *data = (uint32_t*)blk;
   data++;
   for( int i=0; i<(STORE_DATA_SIZE-4)/4; i++ )
      crc->data = *data++;
   return crc->data;
}

// Invalidate a block by zeroing out the first 64-bits
// which includes the CRC and mark
static void Invalidate( uint32_t addr )
{
   uint32_t zero[2] = {0,0};
   FlashWrite( addr, zero, sizeof(zero) );
}
