/* libinit.c
 *
 * Author:  Kai Uwe Rommel <rommel@jonas>
 * Created: Sat Jan 01 1994
 */
 
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

extern char cmdid[];
extern exiterr();

#ifdef main
#undef main
#endif

int main(int argc, char **argv)
{
  /* Before actually starting the RCS programs, additional
   * startup code can be executed here, such as for command
   * line expansion.
   */

  char *rcstz = getenv("RCSTZ");
  char tzbuffer[256];
  int i, j;

  if (rcstz)
  {
    strcpy(tzbuffer, "TZ=");
    strcat(tzbuffer, rcstz);
    putenv(tzbuffer);
  }

#ifdef __EMX__
  _response(&argc, &argv);
  _wildcard(&argc, &argv);
#ifndef __WIN32__
  _emxload_env("RCSLOAD");
#endif
#endif

  for (i = 1; i < argc; i++)
  {
    struct stat statb;
    int e = stat(argv[i], &statb);

    if ((e == 0 && (statb.st_mode & S_IFMT) == S_IFDIR)
	|| strcmp(argv[i], "RCS") == 0)
    {
      for (j = i; j < argc; j++)
	argv[j] = argv[j + 1];
      argc--;
    }
  }

#ifdef RCSDLL
  dllmain(cmdid, exiterr);
#endif

  return rcsmain(argc, argv);
}

#if defined(__IBMC__)

#include <io.h>

int ibmc_rename(char *from, char *to)
{
  char *copy;

  if (*from != ',')
    return rename(from, to);

  copy = _alloca(strlen(from) + 3);
  strcpy(copy, ".\\");
  strcat(copy, from);

  return rename(copy, to);
}

int ibmc_unlink(char *filename)
{
  char *copy;

  if (*filename != ',')
    return remove(filename);

  copy = _alloca(strlen(filename) + 3);
  strcpy(copy, ".\\");
  strcat(copy, filename);

  return unlink(copy);
}

int ibmc_chmod(char *file, int mode)
{
  return chmod(file, mode & 0x0FFF);
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static int std[3] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};

int ibmc_dup2(int fromfd, int tofd)
{
  int rc = dup2(fromfd, tofd);

  if (0 <= tofd && tofd <= 2)
    SetStdHandle(std[tofd], (HANDLE) _get_osfhandle(tofd));

  return rc;
}

#endif

#endif

/* end of libinit.c */
