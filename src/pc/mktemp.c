/* mktemp.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include <conf.h>
#include <string.h>
#include <process.h>
#include <io.h>
#include <errno.h>

char *mktemp (char *string)
{
  int pid, n, saved_errno;
  char *s;

  pid = getpid ();
  s = strchr (string, 0);
  n = 0;
  while (s != string && s[-1] == 'X')
    {
      --s; ++n;
      *s = (char)(pid % 10) + '0';
      pid /= 10;
    }
  if (n < 2)
    return NULL;
  *s = 'a'; saved_errno = errno;
  for (;;)
    {
      errno = 0;
      if (access (string, 0) != 0 && errno == ENOENT)
        {
          errno = saved_errno;
          return string;
        }
      if (*s == 'z')
        {
          errno = saved_errno;
          return NULL;
        }
      ++*s;
    }
}
