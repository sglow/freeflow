#!/usr/bin/python

from PIL import Image, ImageFont, ImageDraw
import sys

def main():
   if( len(sys.argv) < 5 ):
      print 'This utility is used to convert a TrueType font file'
      print 'into a header file for use by my display'
      print
      print 'Usage: font_convert.py <font_file> <size> <header_file> <font_name>'
      print '  font_file   - the name of the font file to open'
      print '  size        - size of the font to create in points'
      print '  header_file - the name of the header file to create'
      print '  font_name   - name of the font structure to write in the header'
      print
      print 'Note that on Linux systems you can usually find the'
      print 'font files in /usr/share/fonts/truetype'
      return
   
   font_file = sys.argv[1]
   point_size = int(sys.argv[2])
   header_name = sys.argv[3]
   font_name = sys.argv[4]

   # Load the font file and create a list of bitmapped characters
   char_list = LoadFont( font_file, point_size )
#   for c in char_list:
#      print
#      print c.ch
#      c.Print()
   
   CreateHeader( header_name, font_name, font_file, point_size, char_list )

# Represents one character in the font
class Char:
   def __init__( self, ch, font ):
      self.ch = ch;
      self.size = font.getsize( ch )

      M = font.getmetrics()
      self.height = M[0]+M[1]

      img = Image.new(mode='1', size=self.size, color=0)
      d = ImageDraw.Draw(img)
      d.text( (0,0), ch, font=font, fill=1 )

      self.img = img
      self.MakeBitmap( self.height )

   # Create an array of byte values that hold the pixel data
   # for this character.
   # Pixels are stored in column order meaning that the first
   # byte of the returned array holds the top 8 rows of the first
   # column, the next byte hold the next 8 rows of that column,
   # etc.
   #
   # Height is the total height of the font.
   def MakeBitmap( self, height ):

      # First, create an integer for each column
      # of the font containing all the bits for that column
      cols = []

      for x in range(self.size[0]):
         C = 0
         mask = 1
         for y in range(self.size[1]):
            if( self.img.getpixel((x,y)) ):
               C |= mask
            mask <<= 1
         cols.append(C)

      # I don't store any initial zero columns
      # rather I just keep track of how many there were
      self.xoff = 0

      for c in cols:
         if( c ):
            break
         self.xoff += 1

      cols = cols[self.xoff:]

      # Discard any empty columns at the end of the font also
      while( len(cols) ):
         if( cols[-1] ):
            break
         cols = cols[:-1]

      # Find how many bytes / column we will have
      # based on the font height
      bpc = (height+7)/8

      self.bitmap = []
      for c in cols:
         col = []
         for i in range(bpc):
            col.append( c & 0xFF )
            c >>= 8

         # I reverse the order of the columns for 
         # convenience in rendering the font
         self.bitmap += col[::-1]

   # Print the font out on the screen,
   # mostly for debugging purposes
   def Print( self ):
      for y in range(self.size[1]):
         S = '|'
         for x in range(self.size[0]):
            if( self.img.getpixel( (x,y) ) ):
               S += '*'
            else:
               S += '.'
         S += '|'
         print S
      for y in range(self.size[1],self.height):
         S = '|'
         for x in range(self.size[0]):
            S += ' '
         S += '|'
         print S

def LoadFont( font_file, point_size ):

   font = ImageFont.truetype( font_file, size=point_size );

   char_list = []

   for i in range( 0x20, 0x7F ):
      char_list.append( Char( chr(i), font ) )

   return char_list


def CreateHeader( header_name, font_name, font_file, point_size, char_list ):
   fp = open( header_name, 'w' )

   fp.write( '// Auto-generated font header\n' )
   fp.write( '// Input font file: %s\n' % font_file )
   fp.write( '// Font point size: %d\n' % point_size )
   fp.write( '\n' )

   # Combine all the character bitmap data into one big
   # list and add it to the header file
   bitmap = []
   for c in char_list:
      bitmap += c.bitmap

   S = 'static const uint8_t %s_bitmap[] = {' % font_name

   for i in range(len(bitmap)):
      if( i & 0xF == 0 ):
         fp.write( S + '\n' )
         S = '   '
      S += '0x%02x, ' % bitmap[i]

   S = S.rstrip()
   if( S[-1] == ',' ):
      S = S[:-1] + ' '

   fp.write( S + '};\n\n' )

   fp.write( 'static const FontChar %s_chars[] = {\n' % font_name )
   off = 0;
   for i in range(len(char_list)):
      c = char_list[i]
      fp.write( "   { %4d, %2d, %2d, %2d },      // '%c'\n" % (off, len(c.bitmap), c.xoff, c.size[0], c.ch  ) )
      off += len(c.bitmap)

   fp.write( '};\n\n' )

   start = ord(char_list[0].ch)
   end = ord(char_list[-1].ch)
   height = char_list[0].height
   fp.write( 'static const FontInfo %s = { %s_bitmap, %s_chars, %d, %d, %d };\n' % (font_name, font_name, font_name, height, start, end) )
   fp.close()

main()
