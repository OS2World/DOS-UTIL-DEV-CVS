/* strippath.c -- remove unnecessary components from a path specifier
   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#if defined(STDC_HEADERS) || defined(USG)
#include <string.h>
#ifndef index
#define	index strchr
#endif
#else
#include <strings.h>
#endif

#include <stdio.h>

#if __STDC__
static void remove_component(char *beginc, char *endc);
void strip_trailing_slashes(char *path);
#else
static void remove_component();
void strip_trailing_slashes();
#endif /* __STDC__ */

#if defined(OS2) || defined(DOS)
/* OS/2 can generally use / too, but not in some places when commands
 * are passed to CMD.EXE for execution (i.e. I/O redirection). */
#define DIRSEP           '\\'
#define DIRSEPSTR        "\\"
#define ISDIRSEP(c)      ((c) == DIRSEP || (c) == '/')
extern char *index_sep(char *);
extern char *rindex_sep(char *);
/*#define system(c)        (flushall(),system(c))*/
#else
#define DIRSEP           '/'
#define DIRSEPSTR        "/"
#define ISDIRSEP(c)      ((c) == DIRSEP)
#define index_sep(p)     index(p, DIRSEP)
#define rindex_sep(p)    rindex(p, DIRSEP)
#define stricmp          strcmp
#define strnicmp         strncmp
#endif

/* Remove unnecessary components from PATH. */

void
strip_path (path)
     char *path;
{
  int stripped = 0;
  char *cp, *slash;

  for (cp = path; (slash = index_sep(cp)) != NULL; cp = slash)
    {
      *slash = '\0';
      if ((!*cp && (cp != path || stripped)) ||
	  strcmp(cp, ".") == 0 || strcmp(cp, DIRSEPSTR) == 0)
	{
	  stripped = 1;
	  remove_component(cp, slash);
	  slash = cp;
	}
      else
	{
	  *slash++ = DIRSEP;
	}
    }
  strip_trailing_slashes(path);
}

/* Remove the component delimited by BEGINC and ENDC from the path */

static void
remove_component (beginc, endc)
     char *beginc;
     char *endc;
{
  for (endc++; *endc; endc++)
    *beginc++ = *endc;
  *beginc = '\0';
}
