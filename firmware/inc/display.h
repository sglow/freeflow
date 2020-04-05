/* display.h */

#ifndef _DEF_INC_DISPLAY
#define _DEF_INC_DISPLAY

#include <stdint.h>

#define FONT_FREESANS_20       0
#define FONT_FREESANS_16       1
#define FONT_FREESANS_12       2

// Represents one character of a font
typedef struct
{
   uint16_t bmOff;       // Offset into bitmap array
   uint8_t  bmLen;       // Number of bytes in bitmap array
   uint8_t  xOff;        // Offset to first populated column
   uint8_t  xAdv;        // Total width of this character
} FontChar;

// Represents an entire font
typedef struct
{
   const uint8_t *bitmap;  // Pointer to bitmap data
   const FontChar *chars;  // Pointer to character array
   uint8_t yAdv;           // Y advance / row
   uint8_t firstChar;      // First character in font.  Normally 0x20, ASCII space
   uint8_t lastChar;       // Last character in font.  Normally 0x7E, ASCII ~
} FontInfo;

// prototypes
void InitDisplay();
void DispISR( void );
int SetFont( uint8_t id );
const FontInfo *GetFontInfo( uint8_t id );
const FontInfo *CrntFont( void );
int DrawChar( uint8_t ch, int x, int y );
int DrawString( const char *str, int x, int y );
void ClearDisplay( void );
void FillRect( int x1, int y1, int w, int h, int color );
void SetPixel( int x, int y );
void ClearPixel( int x, int y );
void UpdateDisplay( void );

#endif
