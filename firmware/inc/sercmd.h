/* sercmd.h */

#ifndef _DEF_INC_SERCMD
#define _DEF_INC_SERCMD

#include <stdint.h>

typedef struct
{
   uint8_t buff[200];
   int8_t  usb;
   int8_t  state;
   uint8_t flag;
   int16_t cmdNdx;
   int16_t rspLen;
} SerCmdInfo;

// prototypes
void InitSerCmd( SerCmdInfo *info, int usb );
void PollSerCmd( SerCmdInfo *info );


#endif
