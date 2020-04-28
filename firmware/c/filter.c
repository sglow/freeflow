/* filter.c */

#include "filter.h"

void FilterInit( Filter *f, const float A[2], const float B[3] )
{
   for( int i=0; i<2; i++ ) f->a[i] = A[i];
   for( int i=0; i<3; i++ ) f->b[i] = B[i];
   FilterClear( f );
}

float FilterFlt( float xn, Filter *f )
{
   float yn;
   float *a = f->a;
   float *b = f->b;
   float *x = f->x;
   float *y = f->y;

   yn = b[0] * xn + b[1]*x[0] + b[2]*x[1] - a[0]*y[0] - a[1]*y[1];

   x[1] = x[0];
   x[0] = xn;
   y[1] = y[0];
   y[0] = yn;

   return yn;
}

void FilterClear( Filter *f )
{
   f->x[0] = 0.0f;
   f->x[1] = 0.0f;
   f->y[0] = 0.0f;
   f->y[1] = 0.0f;
}

