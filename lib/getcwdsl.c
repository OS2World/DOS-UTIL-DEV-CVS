/* getcwdsl - like getcwd, except replace / with \\ in pathnames */

	/* $Id: getcwdsl.c 1.2 1993/03/14 20:14:41 Erik Exp $ */

#include <stdlib.h>

#ifndef __GNUC__
char *getcwd(char*, size_t);
#endif

char * getcwdsl(char *buf, size_t size)
{
#if defined(OS2)
    char *g = _getcwd2(buf, size), *p;
#else
    char *g = getcwd(buf, size), *p;
#endif
    if (g)
		for (p = g;  *p;  p++)
			if (*p == '/')
				*p = '\\';
	return g;
}
