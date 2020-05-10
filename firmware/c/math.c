
#include <stdint.h>
#include <math.h>
#include "utils.h"

#define HUGE_VAL        3.4028234e38

/*
 * IEEE 32-bit single precision floating point numbers are represented:
 *
 *  S EEEEEEEE FFFFFFFFFFFFFFFFFFFFFFF
 *  . ........ \\\\\\\\\\\\\\\\\\\\\\\ - Fraction (23 bits)
 *  . \\\\\\\\-------------------------- Exponent (8 bits)
 *  \----------------------------------- Sign 
 *
 * Some special values:
 *  Nan: E=255, F!=0 
 * +Inf: E=255, F=0, S=0
 * -Inf: E=255, F=0, S=1
 */
int isnanf( float f )
{
   int X = *((int*)&f);
   int F = X & 0x007FFFFF;
   int E = X & 0x7F800000;

   return (E==0x7F800000) && (F != 0);
}

int isinff( float f )
{
   int X = *((int*)&f);
   int F = X & 0x007FFFFF;
   int E = X & 0x7F800000;

   if( F ) return 0;
   if( E != 0x7F800000 ) return 0;
   return (f<0) ? -1 : 1;
}

float floorf( float in )
{
   float x = (int)in;
   if( in >= 0 ) return x;
   if( x == in ) return x;
   return x-1;
}

/****************************************************************************
 *  POW() - Power
 *
 *  z = mantissa x
 *  log2(z) = c1 * z ^ 9 + c2 * z ^ 7 + c3 * z ^ 5 + c4 * z ^ 3 + c5 * z
 *  log2(x) = exponent x + log2(z)
 *  a = y * log2(x)
 *  b = mantissa a
 *  2 ^ b =  (((((d1 * b + d2) * b + d3) * b + d4) * b + d5) * b + d6)
 *
 *  result = 2 ^ exponent a * 2 ^ b
 *         = 2 ^ (y * log2(x))
 ****************************************************************************/
float powf(float x, float y)
{
    float x2;
    int n;
    int sign = 0;
    int iy = (int)y;

    /* 0 ^^ n, n < 0 => 1 / 0 ^^ n => 1 / 0 => error */
    if ((x == 0.0) && (y <= 0.0))
        return 0.0;
    
    /* x ^^ 0 = 1.0 */
    if (y == 0.0) return (1.0);
    
    /* x ^^ 1 = x */
    if (y == 1.0) return (x);
    
    /* 1 ^^ y = 1.0 */
    if (x == 1.0) return(1.0);
    
    /* -1 ^^ y = -1.0 if y is odd integer, return 1.0 if y is even. */
    /* -1 ^^ 0 handled before we get here.                          */
    if ((x == -1.0) && (y == iy)) return (iy & 1) ? -1.0 : 1.0;
    
    /* 0 ^^ n => 0, n > 0 */
    if (x == 0.0)
        return 0.0;
    
    /* 2 ^^ y (times 1.0)*/
    if ((x == 2.0) && (y == iy)) return ldexpf(1.0, iy);
    if ((x == -2.0) && (y == iy)) return (iy & 1) ? -ldexpf(1.0, iy) : ldexpf(1.0, iy);
    
    if (x < 0.0)
    {
        /*****************************************************************/
        /* if y is not an integer, a domain error occurs                 */
        /*****************************************************************/
        if ((y - iy) != 0.0) 
            return 0.0; 
        
        /*****************************************************************/
        /* if x < 0, compute the power of |x|                            */
        /*****************************************************************/
        x = -x;
        
        /*****************************************************************/
        /* for odd exponents, negate the answer                          */
        /*****************************************************************/
        if (iy & 1)
            sign = 1;
    }
    
    /***************************************************************/
    /* x = mantissa of x, n = exponent of x                        */
    /***************************************************************/
    x = 2.0 * frexpf(x, &n);
    --n;
    
    /***************************************************************/
    /* log2(x) is approximately (x - sqrt(2) / x + sqrt(2))        */
    /***************************************************************/
#define SQRTWO 1.4142135623730951
#define L1        2.885390072738
#define L3        0.961800762286
#define L5        0.576584342056
#define L7        0.434259751292
#define T6        0.0002082045327
#define T5        0.001266912225
#define T4        0.009656843287
#define T3        0.05549288453
#define T2        0.2402279975
#define T1        0.6931471019
    x  = (x - SQRTWO) / (x + SQRTWO);
    x2 = x * x;
    
    /***************************************************************/
    /* polynomial expansion for log2(x)                            */
    /***************************************************************/
    x = ((((L7 * x2 + L5) * x2 + L3) * x2 + L1) * x + 0.5);
    
    /***************************************************************/
    /* log2(x) = log2(mantissa(x)) + exponent(x)                   */
    /***************************************************************/
    x += n;
    
    /***************************************************************/
    /* x = y * log2(x)                                             */
    /* n = integer part of x                                       */
    /* x = fractional part of x                                    */
    /***************************************************************/
    x *= y;
    n  = x;
    if ((x -= n) < 0) 
    { 
        x += 1.0; 
        n -= 1; 
    }
    
    /***************************************************************/
    /* polynomial expansion for 2 ^ x                              */
    /***************************************************************/
    x = ((((((T6 * x + T5) * x + T4) * x + T3) * x + T2) * x + T1) * x + 1.0);
    
    x = ldexpf(x, n);
    return (sign) ? -x : x;
}

/****************************************************************************
 *  LOG() - natural log
 *
 *  Based on the algorithm from "Software Manual for the Elementary
 *  Functions", Cody and Waite, Prentice Hall 1980, chapter 5.
 *
 *  N = exponent x
 *  f = mantissa x, 0.5 <= f < 1
 *  if f < sqrt(0.5), znum = f - 0.5, zden = znum * 0.5 + 0.5
 *  if f > sqrt(0.5), znum = f - 1, zden = f * 0.5 + 0.5
 *  z = znum / zden
 *  w = z * z
 *  R = polynomial expression
 *
 *  result = R + N * ln(2)
 ****************************************************************************/
float logf(float x)
{
    int n;
    float a, b, f, r, w, z, znum;

    /************************************************************************/
    /* check for errors in domain and range                                 */
    /************************************************************************/
    if (x <= 0) { return (-HUGE_VAL); }

    /************************************************************************/
    /* f = mantissa(x), n = exponent(x)                            */
    /************************************************************************/
    f = frexpf(x, &n);

    /************************************************************************/
    /* for numbers <= sqrt(0.5)                                             */
    /************************************************************************/
    if (f <= 0.7071067811865476f )
    {
        --n;
        znum = f - 0.5;
        z    = znum / (znum * 0.5 + 0.5);
    }

    /************************************************************************/
    /* for numbers > sqrt(0.5)                                              */
    /************************************************************************/
    else
    {
        znum = (f - 0.5) - 0.5;
        z    = znum / (f * 0.5 + 0.5);
    }

    /************************************************************************/
    /* determine polynomial expression                                      */
    /************************************************************************/
    w = z * z;

#define SQRTHALF  0.70710678118654752440
#define C3        0.693359375
#define C4       -2.121944400546905827679e-4
#define A0       -0.5527074855
#define B0       -0.6632718214e1
    a = A0;
    b = w + B0;

    /************************************************************************/
    /* calculate the natural log of (mant x) / 2                            */
    /************************************************************************/
    r = z + z * w * (a / b);

    /************************************************************************/
    /* ln(x) = ln (mant x) + 2 * (exp x) (but more mathematically stable)   */
    /************************************************************************/
    return ((n * C4 + r) + n * C3);
}

float log10f(float x)
{
   // ln(x) / ln(10)
   return logf(x) * 0.43429448190325176f;
}

float frexpf(float x, int *pw2)
{
   if( x==0 )
   {
      *pw2=0; 
      return 0; 
   }

   uint32_t xi = F2I(x);

   /* Find the exponent (power of 2) */
   int i  = (( xi >> 23) & 0x000000ff) - 0x7e;

   *pw2 = i;
   xi &= 0x807fffff; /* strip all exponent bits */
   xi |= 0x3f000000; /* mantissa between 0.5 and 1 */
   return I2F(xi);
}

float ldexpf( float x, int pw2 )
{
   uint32_t xi = F2I(x);

   int e = (xi>>23) & 0xff;
   e += pw2;

   xi = ((e & 0xff)<<23) | (xi & 0x807fffff);
   return I2F(xi);
}
