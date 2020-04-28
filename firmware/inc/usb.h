/* usb.h */

#ifndef _DEF_INC_USB
#define _DEF_INC_USB

#include <stdint.h>

typedef struct 
{
   uint8_t rqType;      // Request characteristics
   uint8_t rqst;        // Type of the request
   uint16_t value;      // depends on request
   uint16_t index;      // index or offset
   uint16_t len;        // bytes to transfer in data stage
} __attribute__((__packed__)) USBrqst;

// USB device descriptor
typedef struct
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint16_t usbNum;     // USB spec number in BCD
   uint8_t  devClass;   // Device class code
   uint8_t  subClass;   // Device sub-class code
   uint8_t  protocol;   // Protocol code
   uint8_t  maxPkt0;    // Max packet size (bytes) for endpoint 0
   uint16_t vendorID;   // Vendor ID code
   uint16_t productID;  // Product ID code
   uint16_t rev;        // Device release number in BCD
   uint8_t  nMfg;       // String index of manufacturer name
   uint8_t  nProduct;   // String index of product name
   uint8_t  nSerial;    // String index of serial number
   uint8_t  numCfgs;    // Number of device configurations
} __attribute__((__packed__)) USBdesc_Device;

// USB config descriptor
// The other speed descriptor also uses this layout
typedef struct 
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint16_t totLen;     // Total length in bytes including interface and endpoint descriptors
   uint8_t  numIntf;    // Number of interfaces in configuration
   uint8_t  index;      // Index of this configuration
   uint8_t  nCfgName;   // String index of config name
   uint8_t  attrib;     // Configuration attributes bit-map
   uint8_t  maxPwr;     // Max power draw in 2mA units
} __attribute__((__packed__)) USBdesc_Config;

// USB interface descriptor
typedef struct 
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint8_t  index;      // Index of this interface
   uint8_t  altNdx;     // Alternate setting index
   uint8_t  numEP;      // Number of endpoints (not counting ep 0)
   uint8_t  intfClass;  // Interface class code
   uint8_t  subClass;   // Sub-class code
   uint8_t  protocol;   // Protocol code
   uint8_t  nIntf;      // String index of interface name
} __attribute__((__packed__)) USBdesc_Interface;

// USB endpoint descriptor
typedef struct 
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint8_t  addr;       // Endpoint address 
   uint8_t  attrib;     // Attributes bit-map
   uint16_t maxPkt;     // Max packet size
   uint8_t  interval;   // Polling interval 
} __attribute__((__packed__)) USBdesc_Endpoint;

// CDC header descriptor
typedef struct 
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint8_t  subType;    // Descriptor subtype
   uint16_t bcdCDC;     // CDC class version in BCD
} __attribute__((__packed__)) USBdesc_CDC_Header;

// CDC call management descriptor
typedef struct
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint8_t  subType;    // Descriptor subtype
   uint8_t  cap;        // Device capabilities
   uint8_t  dataIntf;   // Data interface
} __attribute__((__packed__)) USBdesc_CDC_CallMgmt;

// CDC Abstract Control Management descriptor
typedef struct
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint8_t  subType;    // Descriptor subtype
   uint8_t  cap;        // Capabilities
} __attribute__((__packed__)) USBdesc_CDC_ACM;

// CDC Union descriptor
typedef struct
{
   uint8_t  length;     // Length of descriptor in bytes
   uint8_t  descType;   // Descriptor type
   uint8_t  subType;    // Descriptor subtype
   uint8_t  master;     // Master interface
   uint8_t  slave;      // Slave interface
} __attribute__((__packed__)) USBdesc_CDC_Union;

// USB string descriptor
#define MAX_USB_STR 64
typedef struct
{
   uint8_t  length;              // Length of descriptor in bytes
   uint8_t  descType;            // Descriptor type
   uint16_t chr[MAX_USB_STR];    // Unicode characters or language codes for string 0
} __attribute__((__packed__)) USBdesc_String;

// prototypes
void InitUSB( void );
void PollUSB( void );
int USB_SendByte( uint8_t dat );
int USB_Send( uint8_t dat[], int ct );
int USB_Recv( void );
int USB_TxFree( void );


#endif

