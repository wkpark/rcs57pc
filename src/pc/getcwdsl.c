/* getcwdsl - like getcwd, except replace \ with / in pathnames */

	/* $Id: getcwdsl.c,v 1.1 1992/07/28 16:13:03 eggert Exp $ */

#include <sys/types.h>
#include <stddef.h>

#ifdef __EMX__
#define getcwd _getcwd2
#endif

#ifdef __IBMC__
#include <direct.h>
#else
char *getcwd(char*, size_t);
#endif

char *
getcwdsl(char *buf, size_t size)
{
	char *g = getcwd(buf, size), *p;
	if (g)
		for (p = g;  *p;  p++)
			if (*p == '\\')
				*p = '/';
	return g;
}
