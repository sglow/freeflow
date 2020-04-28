/* ascii.c */

#include "ascii.h"
#include "string.h"

// At some point I'm planning to add a simple ASCII command interface
// That's still in the future though, for now this is just a place holder
int ProcessAsciiCmd( char *cmdBuff, int max )
{
   // Trim white space from command
//   char *cmd = trim( (char*)cmdBuff );

   strcpy( cmdBuff, "nothing to see here\n" );
   return strlen(cmdBuff);
}
