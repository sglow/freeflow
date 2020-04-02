/* display.h */

#ifndef _DEF_INC_DISPLAY
#define _DEF_INC_DISPLAY

// Multi-byte commands
#define DISP_MCMD_SET_CONTRAST            0x81

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
void PollDisplay();
void DrawChar( uint8_t ch, int x, int y, const FontInfo *fptr );

#endif
