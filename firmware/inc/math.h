/* math.h */

#ifndef _DEF_INC_MATH
#define _DEF_INC_MATH

static inline float sqrtf( float in )
{
   float ret;
   asm volatile ( "vsqrt.f32 %[output], %[input]": [output] "=t" (ret) : [input] "t" (in) );
   return ret;
}

static inline float fabsf( float in )
{
   float ret;
   asm volatile ( "vabs.f32 %[output], %[input]": [output] "=t" (ret) : [input] "t" (in) );
   return ret;
}

#endif
