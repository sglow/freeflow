// The plan is to rename this stdio.h once I've got my sprintf debugged

#ifndef _DEF_INC_SPRINTF
#define _DEF_INC_SPRINTF

#include <stdarg.h>
#include <stddef.h>

int sprintf( char *str, const char *format, ... );
int snprintf( char *str, size_t size, const char *format, ... );
int vsnprintf( char *str, size_t size, const char *format, va_list ap );


#endif
