/* spawnvpq for MS-DOS and OS/2 */

/* by Paul Eggert and Frank Whaley */

	/* $Id: spawnvpq.c,v 1.5 1992/02/17 23:02:20 eggert Exp $ */

#if !defined(__EMX__) || defined(__MSDOS__)

#include <conf.h>
#include <errno.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <sys/types.h>

/*
* Most MS-DOS and OS/2 versions of spawnvp do not
* properly handle arguments with embedded blanks,
* so spawnvpq works around the bug by quoting arguments itself.
* The quoting regime, bizarre as it sounds, is as follows:
*
*	If an argument contains N (>=0) backslashes followed by '"',
*	precede the '"' with an additional N+1 backslashes.
*
*	Surround an argument with '"' if it contains space or tab.
*
*	If an argument contains space or tab and ends with N backslashes,
*	append an additional N backslashes.
*/

int spawnvpq(int mode, char const *path, char * const *argv)
{
	char *a, **argw, **aw, *b, *buf;
	char * const *av;
	size_t argsize = 0, argvsize;

	for (av = argv;  (a = *av++);  ) {
		size_t backslashrun = 0, quotesize = 0;
		char *p = a;
		for (;;) {
			switch (*p++) {
				case '\t': case ' ':
					quotesize = 2;
					/* fall into */
				case '"':
					argsize += backslashrun + 1;
					/* fall into */
				default:
					backslashrun = 0;
					continue;

				case '\\':
					backslashrun++;
					continue;

				case 0:
					if (quotesize)
						argsize += backslashrun;
					break;
			}
			break;
		}
		argsize += p - a + quotesize;
	}

	argvsize = (av-argv) * sizeof(char*);
	if (!(buf  =  malloc(argvsize + argsize))) {
		errno = E2BIG;
		return -1;
	}
	aw = argw = (char**)buf;
	b = buf + argvsize;

	for (av = argv;  (a = *av++);  ) {
		char c;
		int contains_white = strchr(a, ' ') || strchr(a, '\t');
		size_t backslashrun = 0;
		char *p = a;
		*aw++ = b;
		if (contains_white)
			*b++ = '"';
		for (;  ;  *b++ = c) {
			switch ((c = *p++)) {
				case '\\':
					backslashrun++;
					continue;

				case '"':
					backslashrun++;
					memset(b, '\\', backslashrun);
					b += backslashrun;
					/* fall into */
				default:
					backslashrun = 0;
					continue;

				case 0:
					break;
			}
			break;
		}
		if (contains_white) {
			memset(b, '\\', backslashrun);
			b += backslashrun;
			*b++ = '"';
		}
		*b++ = 0;
	}
	*aw = 0;

	{
		int r = spawnvp(mode, path, argw);
		int e = errno;
		free(buf);
		errno = e;
		return r;
	}
}

#endif
