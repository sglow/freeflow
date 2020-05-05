/* usb.c */

#include "buffer.h"
#include "cpu.h"
#include "errors.h"
#include "string.h"
#include "timer.h"
#include "trace.h"
#include "usb.h"
#include "utils.h"

// Endpoint status bit values
#define EPSTAT_DISABLED    0
#define EPSTAT_STALL       1
#define EPSTAT_NAK         2
#define EPSTAT_VALID       3

// local functions
static void HandleReset( void );
static void HandleXfer( void );
static void HandleSetup( int N );
static void ClearEpStat( int N, uint32_t stat );
static void SetEpTxStat( int ndx, int stat );
static void SetEpRxStat( int ndx, int stat );
static int InitTblEntry( int N, int offset, int txLen, int rxLen );

typedef struct
{
   uint8_t state;
   uint16_t size;
   uint16_t remain;
   const uint8_t *data;
} EPinfo;

// local data
static EPinfo epInfo[4];
static uint8_t address;
static CircBuff txBuff, rxBuff;

void InitUSB( void )
{
   // Assign PA11 and PA12 to the USB peripherial
   GPIO_PinAltFunc( DIGIO_A_BASE, 11, 10 );
   GPIO_PinAltFunc( DIGIO_A_BASE, 12, 10 );

   // Enable clock recovery to synchronize our 48MHz internal
   // clock using the USB sync pulse.  This is the default configuration
   // so I really just need to enable it.
   CRS_Regs *crs = (CRS_Regs *)CRS_BASE;
   crs->ctrl = 0x00002060;

   USB_Regs *usb = (USB_Regs *)USBFS_BASE;

   // Clear the power down bit in the USB control register 
   // which is set by default.  This powers up the USB circuitry
   usb->ctrl = 0x00000001;

   // We need to give the analog section time to start up before
   // enabling the USB peripherial.  Frustratingly, the startup time
   // doesn't seem to be in the data sheet, so I'll just wait 1ms
   // which should be plenty I would think.
   BusyWait( 1000 );

   // Take the USB module out of reset and clear any bogus interrupts
   usb->ctrl = 0x00000000;

   // Clear the status.  This doesn't seem to clear if I don't delay 
   // briefly first.
   BusyWait( 10 );
   usb->status = 0;

   // Clear the USB shared RAM area.  This isn't really necessary, 
   // but nice for debugging.
   uint16_t *usbRAM = (uint16_t*)USB_SRAM_BASE;
   for( int i=0; i<512; i++ ) usbRAM[i] = 0;

   epInfo[0].size   = 64;
   epInfo[0].remain = 0;

   // The USB module has a special RAM area where data is transfered.
   // The start of this buffer holds a table of pointers and sizes
   // used to assign the remaining memory to the various USB endpoints.

   int off = 4*sizeof(USB_TblEntry);
   off = InitTblEntry( 0, off, 64, 64 );
   off = InitTblEntry( 1, off,  8,  0 );
   off = InitTblEntry( 2, off,  0, 64 );
   off = InitTblEntry( 3, off, 64,  0 );

   // Enable the pull-up resistor on the DP line
   usb->battery = 0x8000;
}

int USB_SendByte( uint8_t dat )
{
   return BuffAddByte( &txBuff, dat );
}

int USB_Send( uint8_t dat[], int ct )
{
   return BuffAdd( &txBuff, dat, ct );
}

int USB_Recv( void )
{
   return BuffGetByte( &rxBuff );
}

int USB_TxFree( void )
{
   return BuffFree( &txBuff );
}

void PollUSB( void )
{
   USB_Regs *usb = (USB_Regs *)USBFS_BASE;

   if( (usb->endpoint[3] & 0x0030) != 0x0030 )
   {
      USB_TblEntry *usbTbl = (USB_TblEntry *)USB_SRAM_BASE;

      int ct = BuffUsed( &txBuff );
      int wct = (ct+1)/2;

      uint16_t *ptr =  (uint16_t *)(USB_SRAM_BASE + usbTbl[3].txAddr);

      for( int i=0; i<wct; i++ )
      {
         uint16_t l = BuffGetByte( &txBuff );
         uint16_t h = BuffGetByte( &txBuff );
         if( h & 0xFF00 ) h = 0;
         *ptr++ = (h<<8) | l;
      }

      if( ct )
      {
         usbTbl[3].txCount = ct;
         SetEpTxStat( 3, EPSTAT_VALID );
      }
   }

   if( usb->status )
   {
      // Handle a reset event.  We'll get one of these
      // when the USB cable is plugged in.
      if( usb->status & 0x0400 )
      {
         HandleReset();
         return;
      }

      usb->status = 0;
   }

   if( usb->status & 0x8000 )
      HandleXfer();
}

static void HandleReset( void )
{
   USB_Regs *usb = (USB_Regs *)USBFS_BASE;

   usb->status = 0;

   usb->endpoint[0]  = 0x0200;
   SetEpRxStat( 0, EPSTAT_VALID );
   SetEpTxStat( 0, EPSTAT_NAK );

   usb->addr = 0x80;
}

// Clear a status bit in the endpoint register
static void ClearEpStat( int N, uint32_t stat )
{
   USB_Regs *usb = (USB_Regs *)USBFS_BASE;

   uint32_t val = usb->endpoint[N];

   // Clear toggle bits and preserve rw bits
   val &= 0x070F;

   // Write 1s to status bits we don't want to clear
   val |= 0x8080;

   // Clear the status bit passed in
   val &= ~stat;

   usb->endpoint[N] = val;
}

// Update tx/rx status bits in the annoyingly complex endpoint register
static void SetEpTxRxStat( int ndx, int stat, int shift )
{
   USB_Regs *usb = (USB_Regs *)USBFS_BASE;

   uint16_t val = usb->endpoint[ndx];

   // Set the two write clear bits to avoid changing them
   val |= 0x8080;

   // Mask of all toggle bits
   uint16_t tglMask = 0x7070;

   // Clear the two status bits we're possibly changing
   tglMask &= ~(3<<shift);

   // Clear any remaining toggle bits from the value we write to avoid changing them
   val &= ~tglMask;

   // Change the status bits we care about
   val ^= (stat<<shift);

   // Finally, write this back to the endpoint register
   usb->endpoint[ndx] = val;
}

static void SetEpTxStat( int ndx, int stat )
{
   SetEpTxRxStat( ndx, stat, 4 );
}

static void SetEpRxStat( int ndx, int stat )
{
   SetEpTxRxStat( ndx, stat, 12 );
}

static int SendEP( int N )
{
   USB_TblEntry *usbTbl = (USB_TblEntry *)USB_SRAM_BASE;
   uint16_t *sramPtr = (uint16_t*)(USB_SRAM_BASE + usbTbl[N].txAddr);

   EPinfo *ep = &epInfo[N];

   int ct = ep->remain;
   if( ct > ep->size )
      ct = ep->size;

   int wct = (ct+1)/2;
   for( int i=0; i<wct; i++ )
      sramPtr[i] = b2u16( &ep->data[2*i] );

   usbTbl[N].txCount = ct;
   ep->remain -= ct;

   // If there's nothing left to send and I didn't fill the
   // transmit buffer, clear the data pointer to indicate that
   // there's nothing left to send.
   if( !ep->remain && (ct < ep->size) )
      ep->data = 0;
   else
      ep->data += ct;

   SetEpTxStat( N, EPSTAT_VALID );
   return 0;
}

// Start sending data on endpoint 0.
static int StartResp( int N, const void *data, int sz, int max )
{
   if( sz > max ) sz = max;
   epInfo[N].data = (const uint8_t*)data;
   epInfo[N].remain = sz;
   return SendEP( N );
}

static int HandleGetStatus( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleClearFeature( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleReserved( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleSetFeature( USBrqst *rqst ){ return ERR_RANGE; }

static int HandleSetAddress( USBrqst *rqst )
{
   address = (rqst->value & 0x7F);
   return StartResp( 0, 0, 0, 0 );
}

// USB device descriptor
static const USBdesc_Device deviceDesc = 
{
   sizeof(USBdesc_Device),  // length
   0x01,                    // Descriptor type
   0x0200,                  // USB spec number in BCD
   0x02,                    // Device class code
   0x00,                    // Device sub-class code
   0x00,                    // Protocol code
   64,                      // Max packet size (bytes) for endpoint 0
   0x2b74,                  // Vendor ID code
   0x4000,                  // Product ID code
   0x0001,                  // Device release number in BCD
   0x01,                    // String index of manufacturer name
   0x02,                    // String index of product name
   0x00,                    // String index of serial number
   0x01                     // Number of device configurations
};

// Configuration descriptor.
// This consists of the configuration descriptor followed
// by interface descriptor(s) and endpoint descriptor(s)
// for each interface

typedef struct
{
   USBdesc_Config         config;      // Configuration descriptor
   USBdesc_Interface      cdcComm;     // CDC communication interface
   USBdesc_CDC_Header     cdcHeader;   // Has CDC version info
   USBdesc_CDC_CallMgmt   cdcCallMgt;  // CDC call management
   USBdesc_CDC_ACM        cdcAcm;      // CDC abstract control management
   USBdesc_CDC_Union      cdcUnion;    // CDC Union 
   USBdesc_Endpoint       ctrl;        // Control endpoint
   USBdesc_Interface      cdcData;     // CDC data interface
   USBdesc_Endpoint       out;         // Data out endpoint
   USBdesc_Endpoint       in;          // Data in endpoint
} __attribute__((__packed__)) FullCfgDesc;

static const FullCfgDesc configDesc =
{
   // Configuration descriptor
   {
      sizeof(USBdesc_Config),         // Length of descriptor in bytes
      0x02,                           // Descriptor type
      sizeof(FullCfgDesc),            // Total length in bytes including interface and endpoint descriptors
      0x02,                           // Number of interfaces in configuration
      0x01,                           // Index of this configuration
      0x00,                           // String index of config name
      0x80,                           // Configuration attributes bit-map
      100                             // Max power draw in 2mA units
   },

   // CDC communication interface
   { 
      sizeof(USBdesc_Interface),      // Length of descriptor in bytes
      0x04,                           // Descriptor type
      0x00,                           // Index of this interface
      0x00,                           // Alternate setting index
      0x01,                           // Number of endpoints (not counting ep 0)
      0x02,                           // Interface class code (CDC)
      0x02,                           // Sub-class code (abstract control module)
      0x01,                           // Protocol code (V.250)
      0x00,                           // String index of interface name
   },

   // CDC header
   {
      sizeof(USBdesc_CDC_Header),     // Length of descriptor in bytes
      0x24,                           // Descriptor type
      0x00,                           // Descriptor subtype
      0x0101,                         // CDC class version in BCD
   },

   // CDC call management
   {
      sizeof(USBdesc_CDC_CallMgmt),   // Length of descriptor in bytes
      0x24,                           // Descriptor type
      0x01,                           // Descriptor subtype
      0x00,                           // Capabilities 
      0x01,                           // Data interface
   },

   // CDC abstract control management
   {
      sizeof(USBdesc_CDC_ACM),        // Length of descriptor in bytes
      0x24,                           // Descriptor type
      0x02,                           // Descriptor subtype
      0x00,                           // Capabilities 
   },

   // CDC Union 
   {
      sizeof(USBdesc_CDC_Union),      // Length of descriptor in bytes
      0x24,                           // Descriptor type
      0x06,                           // Descriptor subtype
      0x00,                           // Master interface
      0x01,                           // Slave interface
   },

   // Control endpoint
   {
      sizeof(USBdesc_Endpoint),       // Length of descriptor in bytes
      0x05,                           // Descriptor type
      0x81,                           // Endpoint address 
      0x03,                           // Attributes bit-map
      8,                              // Max packet size
      0xff,                           // Polling interval 
   },

   // CDC data interface
   { 
      sizeof(USBdesc_Interface),      // Length of descriptor in bytes
      0x04,                           // Descriptor type
      0x01,                           // Index of this interface
      0x00,                           // Alternate setting index
      0x02,                           // Number of endpoints 
      0x0A,                           // Interface class code (CDC data)
      0x00,                           // Sub-class code 
      0x00,                           // Protocol code
      0x00,                           // String index of interface name
   },

   // Output endpoint
   {
      sizeof(USBdesc_Endpoint),       // Length of descriptor in bytes
      0x05,                           // Descriptor type
      0x02,                           // Endpoint address 
      0x02,                           // Attributes bit-map
      64,                             // Max packet size
      0x01,                           // Polling interval 
   },

   // Input endpoint
   {
      sizeof(USBdesc_Endpoint),       // Length of descriptor in bytes
      0x05,                           // Descriptor type
      0x83,                           // Endpoint address 
      0x02,                           // Attributes bit-map
      64,                             // Max packet size
      0x01,                           // Polling interval 
   }
};

static const char *strings[] = 
{
   "Embedded Intelligence, Inc",
   "Freeflow flow sensor",
};

static int HandleGetDescriptor( USBrqst *rqst )
{
   if( rqst->rqType != 0x80 ) return ERR_RANGE;

   int desc = rqst->value >> 8;
   uint8_t ndx = rqst->value & 0x00FF;

   switch( desc )
   {
      // Device descriptor
      case 0x01:
         if( ndx ) return ERR_RANGE;
         return StartResp( 0, &deviceDesc, sizeof(deviceDesc), rqst->len );

      // Config descriptor
      case 0x02:
         if( ndx ) return ERR_RANGE;
         return StartResp( 0, &configDesc, sizeof(configDesc), rqst->len );

      // String descriptor
      case 0x03:
      {
         static USBdesc_String strDesc;
         strDesc.descType = 3;

         // String index 0 is special, it returns language codes
         if( !ndx )
         {
            strDesc.length = 6;
            strDesc.chr[0] = 0x0409;
            strDesc.chr[1] = 0;
         }
         else if( ndx <= ARRAY_CT(strings) )
         {
            const char *str = strings[ndx-1];
            int l = strlen( str );
            if( l > MAX_USB_STR ) l = MAX_USB_STR;
            for( int i=0; i<l; i++ )
               strDesc.chr[i] = str[i];
            strDesc.length = 2*l + 2;
         }
         else 
            return ERR_RANGE;

         return StartResp( 0, &strDesc, strDesc.length, rqst->len );
      }

      default:
         return ERR_RANGE;
   }

   return ERR_RANGE;
}

static int HandleSetConfig( USBrqst *rqst )
{
   if( rqst->rqType != 0x00 ) return ERR_RANGE;
   if( rqst->value > 1 ) return ERR_RANGE;

   USB_Regs *usb = (USB_Regs *)USBFS_BASE;
   if( !usb->addr ) return ERR_RANGE;

   usb->endpoint[1]  = 0x0301;
   usb->endpoint[2]  = 0x0002;
   usb->endpoint[3]  = 0x0003;

   SetEpRxStat( 1, EPSTAT_NAK );
   SetEpTxStat( 1, EPSTAT_NAK );

   SetEpRxStat( 2, EPSTAT_VALID );
   SetEpTxStat( 2, EPSTAT_NAK );

   SetEpRxStat( 3, EPSTAT_NAK );
   SetEpTxStat( 3, EPSTAT_NAK  );

   return StartResp( 0, 0, 0, 0 );
}

static int HandleSetDescriptor( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleGetConfig( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleGetInterface( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleSetInterface( USBrqst *rqst ){ return ERR_RANGE; }
static int HandleSynchFrame( USBrqst *rqst ){ return ERR_RANGE; }

typedef int (*RqstHandler)( USBrqst *rqst );
static RqstHandler stdRqst[] = 
{
   HandleGetStatus,                 // 0x00
   HandleClearFeature,              // 0x01
   HandleReserved,                  // 0x02
   HandleSetFeature,                // 0x03
   HandleReserved,                  // 0x04
   HandleSetAddress,                // 0x05
   HandleGetDescriptor,             // 0x06
   HandleSetDescriptor,             // 0x07
   HandleGetConfig,                 // 0x08
   HandleSetConfig,                 // 0x09
   HandleGetInterface,              // 0x0a
   HandleSetInterface,              // 0x0b
   HandleSynchFrame,                // 0x0c
};

static int SetLineCoding( USBrqst *rqst )
{
   return StartResp( 0, 0, 0, 0 );
}
static int GetLineCoding( USBrqst *rqst ){ return ERR_RANGE; }

static int SetLineCtrlState( USBrqst *rqst )
{
   return StartResp( 0, 0, 0, 0 );
}

static RqstHandler classRqst[] = 
{
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   HandleReserved, HandleReserved, HandleReserved, HandleReserved,
   SetLineCoding, GetLineCoding, SetLineCtrlState
};


// Handle a setup transfer to endpoint 0
static void HandleSetup( int N )
{
   USB_TblEntry *usbTbl = (USB_TblEntry *)USB_SRAM_BASE;

   USBrqst *rqst = (USBrqst *)(USB_SRAM_BASE + usbTbl[N].rxAddr);

   // Clear the CTR_RX bit
   ClearEpStat( N, 0x8000 );

   int err;
   switch( rqst->rqType & 0x60 )
   {
      case 0x00:
         if( rqst->rqst <= ARRAY_CT(stdRqst) )
            err = stdRqst[ rqst->rqst ]( rqst );
         else
            err = ERR_RANGE;
         break;

      case 0x20:
         if( rqst->rqst <= ARRAY_CT(classRqst) )
            err = classRqst[ rqst->rqst ]( rqst );
         else
            err = ERR_RANGE;
         break;

      default:
         err = ERR_RANGE;
         break;
   }

   if( err )
   {
      SetEpTxStat( N, EPSTAT_STALL );
      SetEpRxStat( N, EPSTAT_STALL );
   }
   else
      SetEpRxStat( N, EPSTAT_VALID );
}

static void HandleRecv( int N )
{
   ClearEpStat( N, 0x8000 );

DbgTrace( 0x10, N, 0 );
   if( N == 2 )
   {
      USB_TblEntry *usbTbl = (USB_TblEntry *)USB_SRAM_BASE;
DbgTrace( 0x11, usbTbl[N].rxCount, 0 );
      uint8_t *ptr = (uint8_t*)(USB_SRAM_BASE + usbTbl[N].rxAddr);
      BuffAdd( &rxBuff, ptr, usbTbl[N].rxCount & 0x01FF );
      SetEpRxStat( N, EPSTAT_VALID );
   }
}

static void HandleXmit( int N )
{
   USB_Regs *usb = (USB_Regs *)USBFS_BASE;
   ClearEpStat( N, 0x0080 );

   if( epInfo[N].data )
      SendEP( N );

   if( address )
   {
      usb->addr = 0x80 | address;
      address = 0;
   }
}

static void HandleXfer( void )
{
   USB_Regs *usb = (USB_Regs *)USBFS_BASE;
   int ep = usb->status & 0x000F;

if( ep ) DbgTrace( 0x20, ep, usb->status );

   if( usb->endpoint[ep] & 0x8000 )
   {
      if( usb->endpoint[ep] & 0x0800 )
         HandleSetup( ep );
      else
         HandleRecv( ep );
   }

   if( usb->endpoint[ep] & 0x0080 )
      HandleXmit( ep );
}

static int InitTblEntry( int N, int offset, int txLen, int rxLen )
{
   uint16_t rxBlks;
   if( rxLen <= 62 )
      rxBlks = rxLen<<10;
   else
   {
      rxLen &= 0x1E0;
      rxBlks = 0x8000 | ((rxLen-0x20)<<5);
   }

   USB_TblEntry *usbTbl = (USB_TblEntry *)USB_SRAM_BASE;
   usbTbl[N].txAddr  = offset;
   usbTbl[N].rxAddr  = offset+txLen;
   usbTbl[N].txCount = 0;
   usbTbl[N].rxCount = rxBlks;
   return offset + txLen + rxLen;
}

