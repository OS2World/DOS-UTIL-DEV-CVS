/* spawnvpq for MS-DOS and OS/2 */

/* by Paul Eggert and Frank Whaley */

	/* $Id: spawnvpq.c 1.6 1993/07/11 15:36:59 Erik Exp $ */

#define USE_SWAP

#include "cvs.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef USE_SWAP
#include "swap.h"
#endif

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
	int
spawnvpq(int mode, char const *path, char * const *argv)
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
		int e;
#ifdef USE_SWAP_LIB
		int r = swap_spawnvp(mode, path, argw);
#else
#ifdef USE_SWAP
		char *tmpfile = tempnam ("", "SPAWN");
		char exec_ret;
		int r;
		char args[127];
		size_t i;

		strcpy (args, "");
		i = strlen (*argw);
		while (*(++argw) != NULL) {
			if (strlen (args) + strlen (*argw) < 126 - i) {
				strcat (args, " ");
				strcat (args, *argw);
			}
			else {
				free(buf);
				errno = E2BIG;
				return -1;
			}
		}
		r = swap (path, args, &exec_ret, tmpfile);
		switch (r) {
			case SWAP_NO_SHRINK:
				printf ("Unable to shrink DOS memory block.");
				break;
			case SWAP_NO_SAVE:
				printf ("Unable to save program.");
				break;
			case SWAP_NO_EXEC:
				printf ("EXEC call failed.  DOS error is: ");
				switch (exec_ret) {
					case BAD_FUNC:
						printf ("Bad function.\n");
						break;
					case FILE_NOT_FOUND:
						printf ("File not found.\n");
						break;
					case ACCESS_DENIED:
						printf ("Access denied.\n");
						break;
					case NO_MEMORY:
						printf ("Insufficient memory.\n");
						break;
					case BAD_ENVIRON:
						printf ("Bad environment.\n");
						break;
					case BAD_FORMAT:
						printf ("Bad format.\n");
						break;
				}
				break;
		}
		r = r == 0 ? exec_ret : -1;
		free (tmpfile);
#else
		int r = spawnvp(mode, path, argw);
#endif
#endif
		e = errno;
		free(buf);
		errno = e;
		return r;
	}
}
