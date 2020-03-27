/* ascii.c */

#include "ascii.h"
#include "string.h"

int ProcessAsciiCmd( char *cmdBuff, int max )
{
   // Trim white space from command
   char *cmd = trim( (char*)cmdBuff );

   strcpy( cmdBuff, "nothing to see here\n" );
   return strlen(cmdBuff);
}
