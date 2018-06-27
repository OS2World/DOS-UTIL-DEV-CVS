/* mkrmdir.c -- BSD compatible directory functions for System V
   Copyright (C) 1988, 1990 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef STDC_HEADERS
extern int errno;
#endif

/* mkdir and rmdir adapted from GNU tar. */

/* Make directory DPATH, with permission mode DMODE.

   Written by Robert Rother, Mariah Corporation, August 1985
   (sdcsvax!rmr or rmr@uscd).  If you want it, it's yours.

   Severely hacked over by John Gilmore to make a 4.2BSD compatible
   subroutine.	11Mar86; hoptoad!gnu

   Modified by rmtodd@uokmax 6-28-87 -- when making an already existing dir,
   subroutine didn't return EEXIST.  It does now. */

int
_mkdir (dpath, dmode)
     char *dpath;
     int dmode;
{
  int cpid, status;
  struct stat statbuf;

#if defined(OS2) || defined(DOS)
  /* printf( "[_mkdir] try to create %s\n", dpath); */
  return mkdir( dpath);
#else /* not OS/2 */

  if (stat (dpath, &statbuf) == 0)
    {
      errno = EEXIST;		/* stat worked, so it already exists. */
      return -1;
    }

  /* If stat fails for a reason other than non-existence, return error. */
  if (errno != ENOENT)
    return -1;

  cpid = fork ();
  switch (cpid)
    {
    case -1:			/* Cannot fork. */
      return -1;		/* errno is set already. */

    case 0:			/* Child process. */
      /* Cheap hack to set mode of new directory.  Since this child
	 process is going away anyway, we zap its umask.
	 This won't suffice to set SUID, SGID, etc. on this
	 directory, so the parent process calls chmod afterward. */
      status = umask (0);	/* Get current umask. */
      umask (status | (0777 & ~dmode));	/* Set for mkdir. */
      execl ("/bin/mkdir", "mkdir", dpath, (char *) 0);
      _exit (1);

    default:			/* Parent process. */
      while (wait (&status) != cpid) /* Wait for kid to finish. */
	/* Do nothing. */ ;

      if (status & 0xFFFF)
	{
	  errno = EIO;		/* /bin/mkdir failed. */
	  return -1;
	}
      return chmod (dpath, dmode);
    }
#endif /* OS/2 */
}

