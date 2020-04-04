/* sprintf.c */

//#define SPRINTF_TEST

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FLG_ALT_FORM     0x00000001      // Alternate form flag
#define FLG_ZERO_PAD     0x00000002      // Zero pad the value
#define FLG_LEFT_ADJ     0x00000004      // Left adjust the value
#define FLG_ADD_BLANK    0x00000008      // Add a blank space if the value is positive
#define FLG_ADD_SIGN     0x00000010      // Always add a sign character
#define FLG_LEN_CHAR     0x00000020      // character length modifier
#define FLG_LEN_SHORT    0x00000040      // Short length modifier
#define FLG_LEN_LLONG    0x00000080      // Long long length modifier
#define FLG_LEN_LONG     0x00000100      // Long length modifier
#define FLG_LEN_LDBL     0x00000200      // Long double length modifier
#define FLG_UNSIGNED     0x00000400      // Unsigned value

typedef struct
{
   int flags;              // Various flags as defined above
   int width;              // Width of the field
   int prec;               // Precision
   char spec;              // Format specification character
   const char *start;      // Pointer to the % character that starts the format field
} FieldInfo;

// local functions
static const char *ParseFieldFlags( FieldInfo *info, const char *fmt );
static const char *ParseLengthModifier( FieldInfo *info, const char *fmt );
static const char *ParseNextInt( int *iptr, const char *fmt, int dflt );
static int FormatInt( FieldInfo *info, long val, char *str, int max );
static int FormatLong( FieldInfo *info, long long val, char *str, int max );
static int FormatBad( FieldInfo *info, const char *fmt, char *str, int max );
static int FormatStr( FieldInfo *info, char *str, const char *src, int max );

// This returns the number of characters that WOULD have been written if the string was long enough.
int MYvsnprintf( char *str, size_t size, const char *format, va_list ap )
{
   int zeroSize = !size;
   if( !zeroSize ) size--;

   int ret = 0;
   while( *format )
   {
      char ch = *format++;
      if( ch != '%' )
      {
         ret++;
         if( size > 0 )
         {
            *str++ = ch;
            size--;
         }
         continue;
      }

      // The first field after % is the flags field
      FieldInfo finfo;
      memset( &finfo, 0, sizeof(finfo) );
      finfo.start = format-1;
      format = ParseFieldFlags( &finfo, format );

      // Next comes an optional width.  Note that I have to check for '*'
      // here because ap can't be safely passed to lower level functions.
      if( *format == '*' )
      {
         format++;
         finfo.width = va_arg( ap, int );
      }
      else 
         format = ParseNextInt( &finfo.width, format, -1 );

      // Next come's an optional precision
      if( *format == '.' )
      {
         if( *++format == '*' )
         {
            format++;
            finfo.prec = va_arg( ap, int );
         }
         else 
            format = ParseNextInt( &finfo.prec, format, -1 );
      }
      else
         finfo.prec = -1;

      // Next comes an optional length modifier
      format = ParseLengthModifier( &finfo, format );

      // Finally the conversion specifier
      finfo.spec = *format;
      if( *format ) format++;

      size_t len = 0;
      switch( finfo.spec )
      {
         case 'u': case 'x': case 'X':
            finfo.flags |= FLG_UNSIGNED;
            // fall through

         case 'd': case 'i': 
            if( finfo.flags & FLG_LEN_LLONG )
            {
               long long int ll = va_arg( ap, long long int );
               len = FormatLong( &finfo, ll, str, size );
               break;
            }
            long val;
            if( finfo.flags & FLG_LEN_LONG )
               val = va_arg( ap, long );
            else
               val = va_arg( ap, int );
            len = FormatInt( &finfo, val, str, size );
            break;

         case 'p': // pointer
         {
            void *ptr = va_arg( ap, void* );
            finfo.flags |= FLG_UNSIGNED;
            len = FormatInt( &finfo, (uint32_t)ptr, str, size );
            break;
         }

         case 's':
         {
            const char *src = va_arg( ap, const char * );
            len = FormatStr( &finfo, str, src, size );
            break;
         }

         case 'c':
         {
            char ch = va_arg( ap, int );
            len = 1;
            if( size ) *str = ch;
            break;
         }

         case '%':
            len = 1;
            if( size ) *str = '%';
            break;

         default:
            len = FormatBad( &finfo, format, str, size );
            break;
      }
      ret += len;

      if( len <= size )
      {
         size -= len;
         str += len;
      }
      else
      {
         str += size;
         size = 0;
      }
   }

   // Add a terminating null character to the string as long
   // as the size passed in wasn't zero
   if( !zeroSize )
      *str = 0;

   return ret;
}

#ifndef SPRINTF_TEST
int vsnprintf( char *str, size_t size, const char *format, va_list ap )
{
   return MYvsnprintf( str, size, format, ap );
}

int sprintf(char *str, const char *format, ...)
{
   va_list ap;
   va_start( ap, format );
   int ret = vsnprintf( str, 0x7fffffff, format, ap );
   va_end(ap);
   return ret;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
   va_list ap;
   va_start( ap, format );
   int ret = vsnprintf( str, size, format, ap );
   va_end(ap);
   return ret;
}
#endif

// Parse the flags portion of the field
static const char *ParseFieldFlags( FieldInfo *info, const char *fmt )
{
   while( 1 )
   {
      switch( *fmt )
      {
         case '#': info->flags |= FLG_ALT_FORM;  break;
         case '0': info->flags |= FLG_ZERO_PAD;  break;
         case '-': info->flags |= FLG_LEFT_ADJ;  break;
         case ' ': info->flags |= FLG_ADD_BLANK; break;
         case '+': info->flags |= FLG_ADD_SIGN;  break;
         default: return fmt;
      }
      fmt++;
   }
}

static const char *ParseLengthModifier( FieldInfo *info, const char *fmt )
{
   switch( *fmt )
   {
      case 'h': 
         fmt++;
         if( *fmt == 'h' )
         {
            fmt++;
            info->flags |= FLG_LEN_CHAR;
         }
         else
            info->flags |= FLG_LEN_SHORT;
         return fmt;

      case 'l': 
         fmt++;
         if( *fmt == 'l' )
         {
            fmt++;
            info->flags |= FLG_LEN_LLONG;
         }
         else
            info->flags |= FLG_LEN_LONG;
         return fmt;

      case 'L':
         info->flags |= FLG_LEN_LDBL;
         return ++fmt;

      default: return fmt;
   }
}

static inline int MYisdigit( char ch ){ return ((ch>='0') && (ch<='9')); }
static inline int MYstrlen( const char *str )
{
   int L;
   for( L=0; *str; str++, L++ ){}
   return L;
}

// Optional field width.  I load the default if no int was found
static const char *ParseNextInt( int *iptr, const char *fmt, int dflt )
{
   if( !MYisdigit(*fmt) )
   {
      *iptr = dflt;
      return fmt;
   }

   int val = 0;
   do
   {
      val *= 10;
      val += *fmt - '0';
   } while( MYisdigit(*++fmt) );

   *iptr = val;
   return fmt;
}

// Add ct padding chacters to the string, Returns the number actually added
static int AddPadding( char *str, char pad, int ct, int max )
{
   if( max < ct ) ct = max;
   for( int i=0; i<ct; i++ ) *str++ = pad;
   return ct;
}

// Format the integer into *str and return the length.
static int FormatInt( FieldInfo *info, long val, char *str, int max )
{
   unsigned long uval;
   int neg = 0;
   int hex = 0;

   // If the format is unsigned and the length is less then that of a long, 
   // mask off extra bits
   if( info->flags & FLG_UNSIGNED )
   {
      unsigned long mask = (unsigned long)-1;
      if( info->flags & FLG_LEN_CHAR )
         mask = 0x00ff;
      else if( info->flags & FLG_LEN_SHORT )
         mask = (unsigned short)-1;
      else if( !(info->flags & FLG_LEN_LONG) )
         mask = (unsigned int)-1;

      uval = val & mask;
      hex = (info->spec != 'u');
   }
   else if( val < 0 )
   {
      neg = 1;
      uval = -val;
   }
   else
      uval = val;

   char buff[30];
   char *bptr = &buff[30];

   // Add a terminating null character
   *--bptr = 0;

   // Handle zero
   if( !uval )
      *--bptr = '0';

   // Handle hex
   else if( hex )
   {
      char A = (info->spec=='x') ? 'a' : 'A';

      while( uval )
      {
         int x = uval & 0x0F;
         if( x < 10 ) 
            *--bptr = '0'+x;
         else 
            *--bptr = A + x - 10;
         uval >>= 4;
      }

      if( info->flags & FLG_ALT_FORM )
      {
         *--bptr = info->spec;
         *--bptr = '0';
      }
   }

   // Handle decimal
   else
   {
      while( uval )
      {
         *--bptr = (uval%10) + '0';
         uval /= 10;
      }
   }

   // Figure out what sign character I need to add (if any)
   char signChar = 0;
   if( neg )
      signChar = '-';
   else if( info->flags & FLG_ADD_SIGN )
      signChar = '+';
   else if( info->flags & FLG_ADD_BLANK )
      signChar = ' ';

   // See how many padding characters I need to add
   int ct, L = strlen(bptr);
   int ret = 0;

   int pct = info->width - L;
   if( signChar ) pct--;
   if( pct < 0 ) pct = 0;

   // If we're left adjusting or zero padding, the sign character goes first
   if( (info->flags & (FLG_LEFT_ADJ|FLG_ZERO_PAD)) && signChar )
   {
      if( max )
      {
         *str++ = signChar;
         max--;
      }
      ret++;
      signChar = 0;
   }

   // If we aren't left adjusting, the padding goes next
   if( !(info->flags & FLG_LEFT_ADJ) )
   {
      char pad = (info->flags & FLG_ZERO_PAD) ? '0' : ' ';
      ct = AddPadding( str, pad, pct, max );
      max -= ct;
      str += ct;
      ret += pct;
      pct = 0;
   }

   // Add the sign character if there's still one to add
   if( signChar )
   {
      if( max )
      {
         *str++ = signChar;
         max--;
      }
      ret++;
   }

   // The value string goes next
   ct = (L<max) ? L : max;
   strncpy( str, bptr, ct );
   str += ct;
   max -= ct;
   ret += L;

   // If there's still padding, add that now.  This would always be a space
   if( pct )
   {
      ct = AddPadding( str, ' ', pct, max );
      max -= ct;
      str += ct;
      ret += pct;
   }

   return ret;
}

// FIXME - need to add support for this
static int FormatLong( FieldInfo *info, long long val, char *str, int max )
{
   return FormatInt( info, (long )val, str, max );
}

static int FormatBad( FieldInfo *info, const char *fmt, char *str, int max )
{
   int len = fmt - info->start;
   int ct = (len>max) ? max : len;

   for( int i=0; i<ct; i++ )
      *str++ = *info->start++;
   return len;
}

static int FormatStr( FieldInfo *info, char *str, const char *src, int max )
{
   int L = MYstrlen( src );

   // If the precision was passed, limit length to that value
   if( info->prec >= 0 )
   {
      if( L > info->prec )
         L = info->prec;
   }

   // Find how much padding to add
   int pad = info->width - L;
   if( pad < 0 ) pad = 0;

   // Add blanks to left of string
   if( pad && !(info->flags & FLG_LEFT_ADJ) )
   {
      int ct = (pad<max) ? pad : max;
      for( int i=0; i<ct; i++ ) *str++ = ' ';
   }

   // Add string
   int ct = (L<max) ? L : max;
   for( int i=0; i<ct; i++ ) *str++ = *src++;
   max -= ct;

   if( !pad ) return L;

   // Add trailing padding
   if( info->flags & FLG_LEFT_ADJ )
   {
      int ct = (pad<max) ? pad : max;
      for( int i=0; i<ct; i++ ) *str++ = ' ';
   }

   return info->width;
}
