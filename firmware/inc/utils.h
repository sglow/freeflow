

#ifndef _DEF_INC_UTILS
#define _DEF_INC_UTILS

#include <stdint.h>

#ifndef offsetof
#define offsetof(type,member)   (int)(&((type*)0)->member)
#endif

static inline void IntDisable( void )
{
   asm volatile( "cpsid i" );
}

static inline void IntEnable( void )
{
   asm volatile( "cpsie i" );
}

static inline void IntRestore( int p )
{
   if( p ) IntEnable();
}

static inline int IntSuspend( void )
{
   int ret;
   asm volatile( "mrs   %[output], primask\n\t" "cpsid i": [output] "=r" (ret) );
   return (ret==0);
}

static inline int16_t Clip16( int32_t val )
{
   int ret;
   asm volatile( "SSAT %[output], #16, %[input]": [output] "=r" (ret) : [input] "r" (val) );
   return ret;
}

static inline uint16_t Clip16u( uint32_t val )
{
   int ret;
   asm volatile( "USAT %[output], #16, %[input]": [output] "=r" (ret) : [input] "r" (val) );
   return ret;
}

static inline uint32_t F2I( float val ){ return *(uint32_t*)&val; }
static inline float I2F( uint32_t val ){ return *(float*)&val; }

static inline uint16_t b2u16( const uint8_t x[] )
{
   uint16_t a = x[0];
   uint16_t b = x[1];
   return (b<<8)|a;
}

static inline uint32_t b2u32( const uint8_t x[] )
{
   uint32_t a = x[0];
   uint32_t b = x[1];
   uint32_t c = x[2];
   uint32_t d = x[3];
   return a | (b<<8) | (c<<16) | (d<<24);
}

static inline float b2flt( const uint8_t x[] )
{
   return I2F( b2u32(x) );
}

static inline void u16_2_u8( uint16_t val, uint8_t ret[] )
{
   ret[0] = val;
   ret[1] = val>>8;
}

static inline void u32_2_u8( uint32_t val, uint8_t ret[] )
{
   ret[0] = val;
   ret[1] = val>>8;
   ret[2] = val>>16;
   ret[3] = val>>24;
}

static inline void flt_2_u8( float val, uint8_t ret[] )
{
   u32_2_u8( F2I(val), ret );
}

static inline void BitSet( uint32_t bit, uint32_t *var )
{
   IntDisable();
   *var |= bit;
   IntEnable();
}

static inline void BitClr( uint32_t bit, uint32_t *var )
{
   IntDisable();
   *var &= ~bit;
   IntEnable();
}

#define ARRAY_CT(x)      (sizeof(x)/sizeof(x[0]))

#define dbgInt      ((int16_t*)0x20000000)
#define dbgUInt     ((uint16_t*)0x20000000)
#define dbgLong     ((int32_t*)0x20000000)
#define dbgULong    ((uint32_t*)0x20000000)
#define dbgL64      ((int64_t*)0x20000000)
#define dbgU64      ((uint64_t*)0x20000000)
#define dbgFlt      ((float*)0x20000000)
#define dbgU8       ((uint8_t*)0x20000000)
#define dbgI8       ((int8_t*)0x20000000)

#endif
