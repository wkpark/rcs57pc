/* dllmain.c
 *
 * Author:  Kai Uwe Rommel <rommel@jonas>
 * Created: Sat Jan 01 1994
 */
 
static char *rcsid =
"$Id$";
static char *rcsrev = "$Revision$";

/* $Log$ */

#include <string.h>

char cmdid[32];
int (*exiterr_ptr)();

exiterr()
{
  exiterr_ptr();
}

void dllmain(char *cmd_id, int (*exit_err)())
{
  strcpy(cmdid, cmd_id);
  exiterr_ptr = exit_err;
}

/* end of dllmain.c */
