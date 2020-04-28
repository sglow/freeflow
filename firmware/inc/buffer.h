/* buffer.h */

#ifndef _DEF_INC_BUFFER
#define _DEF_INC_BUFFER

#include <stdint.h>

typedef struct
{
   uint8_t head, tail;
   uint8_t buff[128];
} CircBuff;

static int BuffUsed( CircBuff *cb )
{
   int h = cb->head;
   int t = cb->tail;

   int ct = h-t;
   if( ct < 0 ) ct += sizeof(cb->buff);
   return ct;
}

static int BuffFree( CircBuff *cb )
{
   return sizeof(cb->buff) - 1 - BuffUsed( cb );
}

static int BuffAddByte( CircBuff *cb, uint8_t dat )
{
   uint8_t newHead = cb->head + 1;
   if( newHead >= sizeof(cb->buff) ) 
      newHead = 0;

   if( newHead == cb->tail )
      return 0;

   cb->buff[ cb->head ] = dat;
   cb->head = newHead;
   return 1;
}

static int BuffAdd( CircBuff *cb, uint8_t *dat, int ct )
{
   int tot = BuffFree( cb );
   if( tot > ct ) tot = ct;

   for( int i=0; i<tot; i++ )
   {
      cb->buff[ cb->head++ ] = dat[i];
      if( cb->head >= sizeof(cb->buff) )
         cb->head = 0;
   }

   return tot;
}

static int BuffGetByte( CircBuff *cb )
{
   if( cb->head == cb->tail )
      return -1;

   int ret = cb->buff[ cb->tail++ ];
   if( cb->tail >= sizeof(cb->buff) )
      cb->tail = 0;

   return ret;
}

static int BuffGet( CircBuff *cb, uint8_t *dat, int max )
{
   int tot = BuffUsed( cb );
   if( tot > max ) tot = max;

   for( int i=0; i<tot; i++ )
   {
      dat[i] = cb->buff[ cb->tail++ ];
      if( cb->tail >= sizeof(cb->buff) )
         cb->tail = 0;
   }
   return tot;
}


#endif
