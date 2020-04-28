/* filter.h */

#ifndef _DEF_INC_FILTER
#define _DEF_INC_FILTER

#include "utils.h"

typedef struct
{
   float a[2];
   float b[3];
   float x[2];
   float y[2];
} Filter;

// inline functions
static inline float FilterOut( Filter *f ){ return f->y[0]; }

/* prototypes */
void FilterInit( Filter *f, const float A[2], const float B[3] );
float FilterFlt( float xn, Filter *f );
void FilterClear( Filter *f );

#endif

