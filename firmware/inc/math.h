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

float powf( float x, float y );
int isnanf( float f );
int isinff( float f );
float frexpf(float x, int *pw2);
float ldexpf( float x, int pw2 );
float logf(float x);
float log10f(float x);
float floorf( float in );

#endif
