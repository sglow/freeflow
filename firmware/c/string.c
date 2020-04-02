#include <string.h>
#include "utils.h"

void *memset( void *s, int c, int sz )
{
   uint8_t *ptr = (uint8_t*)s;
   for( int i=0; i<sz; i++ )
      *ptr++ = c;
   return s;
}

void *memcpy( void *dest, const void *src, int n )
{
   uint8_t *dptr = (uint8_t*)dest;
   const uint8_t *sptr = (uint8_t*)src;
   for( int i=0; i<n; i++ )
      *dptr++ = *sptr++;
   return dest;
}

// Just like memcpy, but buffers can overlap
void *memmove( void *dest, const void *src, int n )
{
   if( dest <= src )
      return memcpy( dest, src, n );

   uint8_t *dptr = (uint8_t*)dest;
   const uint8_t *sptr = (uint8_t*)src;

   dptr += (n-1);
   sptr += (n-1);

   for( int i=0; i<n; i++ )
      *dptr-- = *sptr--;
   return dest;
}

// Compare two blocks of memory.
int memcmp(const void *s1, const void *s2, int n)
{
   const uint8_t *p1 = (uint8_t*)s1;
   const uint8_t *p2 = (uint8_t*)s2;
   for( int i=0; i<n; i++ )
   {
      int diff = *p1++ - *p2++;
      if( diff ) return diff;
   }
   return 0;
}

int strlen(const char *s)
{
   int len = 0;
   while( *s++ ) len++;
   return len;
}

char *strcpy( char *dest, const char *src )
{
   return strncpy( dest, src, 0x7fffffff );
}

char *strncpy( char *dest, const char *src, int n )
{
   for( int i=0; i<n; i++ )
   {
      dest[i] = src[i];
      if( !src[i] ) break;
   }
   return dest;
}

int strcmp( const char *s1, const char *s2 )
{
   return strncmp( s1, s2, 0x7fffffff );
}

int streq( const char *s1, const char *s2 )
{
   return strcmp( s1, s2 ) == 0;
}

int strncmp( const char *s1, const char *s2, int n )
{
   for( int i=0; i<n; i++ )
   {
      uint8_t c1 = (uint8_t)*s1++;
      uint8_t c2 = (uint8_t)*s2++;
      if( c1 != c2 ) return c1-c2;
      if( !c1 ) break;
   }
   return 0;
}

//int strncasecmp( const char *s1, const char *s2, int n )
//{
//   for( int i=0; i<n; i++ )
//   {
//      int diff = toupper(s1[i]) - toupper(s2[i]);
//      if( diff ) return diff;
//      if( !s1[i] ) break;
//   }
//   return 0;
//}
//
//int strcasecmp( const char *s1, const char *s2 )
//{
//   return strncasecmp( s1, s2, 0x7fffffff );
//}

char *ltrim( char *str )
{
   while( isspace(*str) ) str++;
   return str;
}

char *rtrim( char *buff, int len )
{
   if( len < 0 ) len = strlen(buff);
   while( (len > 0) && isspace( buff[len-1] ) )
      buff[--len] = 0;
   return buff;
}

char *trim( char *str )
{
   str = ltrim(str);
   return rtrim(str, -1);
}

char *strchr( const char *s, int c )
{
   for( ; *s; s++ )
   {
      if( *s == c ) return (char*)s;
   }
   return 0;
}

char *strstr( const char *haystack, const char *needle)
{
   int lh = strlen(haystack);
   int ln = strlen(needle);
   for( int i=0; i<=lh-ln; i++ )
   {
      if( !strncmp( &haystack[i], needle, ln ) )
         return (char*)&haystack[i];
   }
   return 0;
}

int isspace( uint8_t ch ){ return (ch==' ') || (ch=='\t') || (ch=='\r') || (ch=='\n'); }

