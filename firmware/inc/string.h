/* string.h */

#ifndef _DEF_INC_STRING
#define _DEF_INC_STRING

#include <stdint.h>

void *memset( void *s, int c, int sz );
void *memcpy( void *dest, const void *src, int n );
void memcpy32( uint32_t *dest, uint32_t *src, int n );
void *memmove( void *dest, const void *src, int n );
int memcmp(const void *s1, const void *s2, int n);
int memcmp32(const uint32_t *s1, const uint32_t *s2, int n);
int strlen(const char *s);
char *strcpy( char *dest, const char *src );
char *strncpy( char *dest, const char *src, int n );
int strcmp( const char *s1, const char *s2 );
int streq( const char *s1, const char *s2 );
int strncmp( const char *s1, const char *s2, int n );
int strncasecmp( const char *s1, const char *s2, int n );
int strcasecmp( const char *s1, const char *s2 );
char *ltrim( char *str );
char *rtrim( char *str, int len );
char *trim( char *str );
int isspace( uint8_t ch );
char *strchr( const char *s, int c );
char *strstr( const char *haystack, const char *needle);

#endif
