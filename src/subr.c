/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.3 kit.
 * 
 * Various useful functions for the CVS support code.
 */

#include "cvs.h"

#ifdef _MINIX
#undef	POSIX		/* Minix 1.6 doesn't support POSIX.1 sigaction yet */
#endif

#ifndef VPRINTF_MISSING
#if __STDC__
#include <stdarg.h>
#define VA_START(args, lastarg) va_start(args, lastarg)
#else
#include <varargs.h>
#define VA_START(args, lastarg) va_start(args)
#endif
#else
#define va_alist a1, a2, a3, a4, a5, a6, a7, a8
#define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
#endif

#if defined(OS2) || defined(DOS)
#ifdef DOS
#undef const
#endif
#include <process.h>
#endif

#ifndef lint
static char rcsid[] = "@(#)subr.c 1.52 92/03/31";
#endif

#if __STDC__
static void run_add_arg (char *s);
static void run_init_prog (void);
#else
static void run_add_arg ();
static void run_init_prog ();
#endif				/* __STDC__ */

extern char *getlogin ();
extern char *strtok ();

/*
 * Copies "from" to "to". mallocs a buffer large enough to hold the entire
 * file and does one read/one write to do the copy.  This is reasonable,
 * since source files are typically not too large.
 */
void
copy_file (from, to)
    char *from;
    char *to;
{
    struct stat sb;
    struct utimbuf t;
    int fdin, fdout;
    char *buf;

    if (trace)
	(void) fprintf (stderr, "-> copy(%s,%s)\n", from, to);
    if (noexec)
	return;

    if ((fdin = open (from, O_RDONLY|O_BINARY)) < 0)
	error (1, errno, "cannot open %s for copying", from);
    if (fstat (fdin, &sb) < 0)
	error (1, errno, "cannot fstat %s", from);
#if defined(OS2) || defined(DOS)
    if ((fdout = open (to, O_WRONLY|O_TRUNC|O_CREAT|O_BINARY,
	(int) sb.st_mode & 07777)) < 0)
#else
    if ((fdout = creat (to, (int) sb.st_mode & 07777)) < 0)
#endif
	error (1, errno, "cannot create %s for copying", to);

#if defined(DOS)
    if (sb.st_size > 0)
    {
	int count;

	buf = xmalloc (1024);
	while( (count = read (fdin, buf, 1024)) > 0 )
	    if (write (fdout, buf, count) != count){
		error (1, errno, "cannot write file %s for copying", to);
	}
	if (count == -1)
	    error (1, errno, "cannot read file %s for copying", from);
	free (buf);
    }
#else  /* not DOS */
    if (sb.st_size > 0)
    {
	buf = xmalloc ((int) sb.st_size);
	if (read (fdin, buf, (int) sb.st_size) != (int) sb.st_size)
	    error (1, errno, "cannot read file %s for copying", from);
	if (write (fdout, buf, (int) sb.st_size) != (int) sb.st_size
#ifndef FSYNC_MISSING
	    || fsync (fdout) == -1
#endif
	    )
	{
	    error (1, errno, "cannot write file %s for copying", to);
	}
	free (buf);
    }
#endif	/* DOS */
    (void) close (fdin);
    if (close (fdout) < 0)
	error (1, errno, "cannot close %s", to);

    /* now, set the times for the copied file to match those of the original */
    t.actime = sb.st_atime;
    t.modtime = sb.st_mtime;
    (void) utime (to, &t);
}

/*
 * Returns non-zero if the argument file is a directory, or is a symbolic
 * link which points to a directory.
 */
int
isdir (file)
    char *file;
{
    struct stat sb;

    if (stat (file, &sb) < 0)
	return (0);
    return (S_ISDIR (sb.st_mode));
}

/*
 * Returns non-zero if the argument file is a symbolic link.
 */
int
islink (file)
    char *file;
{
#ifdef S_ISLNK
    struct stat sb;

    if (lstat (file, &sb) < 0)
	return (0);
    return (S_ISLNK (sb.st_mode));
#else
    return (0);
#endif
}

/*
 * Returns non-zero if the argument file exists.
 */
int
isfile (file)
    char *file;
{
    struct stat sb;

    if (stat (file, &sb) < 0)
	return (0);
    return (1);
}

/*
 * Returns non-zero if the argument file is readable.
 * XXX - must be careful if "cvs" is ever made setuid!
 */
int
isreadable (file)
    char *file;
{
    return (access (file, R_OK) != -1);
}

/*
 * Returns non-zero if the argument file is writable
 * XXX - muct be careful if "cvs" is ever made setuid!
 */
int
iswritable (file)
    char *file;
{
    return (access (file, W_OK) != -1);
}

/*
 * Open a file and die if it fails
 */
FILE *
open_file (name, mode)
    char *name;
    char *mode;
{
  FILE *fp;

  if ((fp = fopen (name, mode)) == NULL)
    error (1, errno, "cannot open %s (mode %s)", name, mode);
  return (fp);
}

/*
 * Open a file if allowed and return.
 */
FILE *
Fopen (name, mode)
    char *name;
    char *mode;
{
    if (trace)
	(void) fprintf (stderr, "-> fopen(%s,%s)\n", name, mode);
    if (noexec)
	return (NULL);

    return (fopen (name, mode));
}

/*
 * Make a directory and die if it fails
 */
void
make_directory (name)
    char *name;
{
    struct stat buf;

    if (stat (name, &buf) == 0)
    {
	if (S_ISDIR (buf.st_mode))
	{
	    if (access (name, (R_OK | W_OK | X_OK)) == 0)
	    {
		error (0, 0, "Directory %s already exists", name);
		return;
	    }
	    else
	    {
		error (0, 0,
		    "Directory %s already exists but is protected from you",
		       name);
	    }
	}
	else
	    error (0, 0, "%s already exists but is not a directory", name);
    }
    if (!noexec && mkdir (name, 0777) < 0)
	error (1, errno, "cannot make directory %s", name);
}

/*
 * Make a path to the argument directory, printing a message if something
 * goes wrong.
 */
void
make_directories (name)
    char *name;
{
    char *cp;

    if (noexec)
	return;

    if (mkdir (name, 0777) == 0 || errno == EEXIST)
	return;
    if (errno != ENOENT)
    {
	error (0, errno, "cannot make path to %s", name);
	return;
    }
    if ((cp = rindex_sep (name)) == NULL)
	return;
    *cp = '\0';
    make_directories (name);
    *cp++ = DIRSEP;
    if (*cp == '\0')
	return;
    (void) mkdir (name, 0777);
}

/*
 * malloc some data and die if it fails
 */
char *
xmalloc (bytes)
    size_t bytes;
{
    char *cp;

    if (bytes <= 0)
	error (1, 0, "bad malloc size %d", bytes);
    if ((cp = malloc (bytes)) == NULL)
	error (1, 0, "malloc failed");
    return (cp);
}

/*
 * realloc data and die if it fails [I've always wanted to have "realloc" do
 * a "malloc" if the argument is NULL, but you can't depend on it.  Here, I
 * can *force* it.
 */
char *
xrealloc (ptr, bytes)
    char *ptr;
    size_t bytes;
{
    char *cp;

    if (!ptr)
	return (xmalloc (bytes));

    if (bytes <= 0)
	error (1, 0, "bad realloc size %d", bytes);
    if ((cp = realloc (ptr, bytes)) == NULL)
	error (1, 0, "realloc failed");
    return (cp);
}

/*
 * Duplicate a string, calling xmalloc to allocate some dynamic space
 */
char *
xstrdup (str)
    char *str;
{
    char *s;

    if (str == NULL)
	return ((char *) NULL);
    s = xmalloc (strlen (str) + 1);
    (void) strcpy (s, str);
    return (s);
}

/*
 * Change the mode of a file, either adding write permissions, or removing
 * all write permissions.  Adding write permissions honors the current umask
 * setting.
 */
void
xchmod (fname, writable)
    char *fname;
    int writable;
{
    struct stat sb;
    int mode, oumask;

    if (stat (fname, &sb) < 0)
    {
	if (!noexec)
	    error (0, errno, "cannot stat %s", fname);
	return;
    }
#if defined(OS2) || defined(DOS)
    if (writable)
    {
	mode = sb.st_mode | S_IWRITE;
    }
    else
    {
	mode = sb.st_mode & ~S_IWRITE;
    }
#else /* not OS/2 */
    if (writable)
    {
	oumask = umask (0);
	(void) umask (oumask);
	mode = sb.st_mode | ((S_IWRITE | S_IWGRP | S_IWOTH) & ~oumask);
    }
    else
    {
	mode = sb.st_mode & ~(S_IWRITE | S_IWGRP | S_IWOTH);
    }
#endif /* OS/2 */

    if (trace)
	(void) fprintf (stderr, "-> chmod(%s,%o)\n", fname, mode);
    if (noexec)
	return;

    if (chmod (fname, mode) < 0)
	error (0, errno, "cannot change mode of file %s", fname);
}

/*
 * Rename a file and die if it fails
 */
void
rename_file (from, to)
    char *from;
    char *to;
{
    if (trace)
	(void) fprintf (stderr, "-> rename(%s,%s)\n", from, to);
    if (noexec)
	return;

#if defined(OS2) || defined(DOS)
    chmod( to, S_IWRITE);
    unlink( to);
    if (rename (from, to) < 0)
	error (1, errno, "cannot rename file %s to %s", from, to);
#else
    if (rename (from, to) < 0)
	error (1, errno, "cannot rename file %s to %s", from, to);
#endif
}

/*
 * link a file, if possible.
 */
#ifndef LINK_MISSING
int
link_file (from, to)
    char *from, *to;
{
  if (trace)
    (void) fprintf (stderr, "-> link(%s,%s)\n", from, to);
  if (noexec)
    return (0);

  return (link (from, to));
}
#endif

/*
 * unlink a file, if possible.
 */
int
unlink_file (f)
    char *f;
{
    if (trace)
	(void) fprintf (stderr, "-> unlink(%s)\n", f);
    if (noexec)
	return (0);

    return (unlink (f));
}

/*
 * Compare "file1" to "file2". Return non-zero if they don't compare exactly.
 *
 * mallocs a buffer large enough to hold the entire file and does two reads to
 * load the buffer and calls bcmp to do the cmp. This is reasonable, since
 * source files are typically not too large.
 */
int
xcmp (file1, file2)
    char *file1;
    char *file2;
{
    register char *buf1, *buf2;
    struct stat sb;
    off_t size;
    int ret, fd1, fd2, r1, r2;

    if ((fd1 = open (file1, O_RDONLY)) < 0)
	error (1, errno, "cannot open file %s for comparing", file1);
    if ((fd2 = open (file2, O_RDONLY)) < 0)
	error (1, errno, "cannot open file %s for comparing", file2);
    if (fstat (fd1, &sb) < 0)
	error (1, errno, "cannot fstat %s", file1);
    size = sb.st_size;
    if (fstat (fd2, &sb) < 0)
	error (1, errno, "cannot fstat %s", file2);
    if (size == sb.st_size)
    {
	if (size == 0)
	    ret = 0;
	else
	{
#if defined(DOS)
	    /* DOS cant't handle reads with large file sizes */
	    int count, c2;

	    buf1 = xmalloc (1024);
	    buf2 = xmalloc (1024);
	    while( (count=read(fd1, buf1, 1024)) > 0) {
	      if ((c2 = read (fd2, buf2, 1024)) != count) {
		error (1, errno, "cannot read file %s for comparing", file2);
	      }
	      if( (ret = bcmp (buf1, buf2, count)) != 0) break;
	    }
	    if (count == -1)
		error (1, errno, "cannot read file %s for comparing", file1);
	    free (buf1);
	    free (buf2);
#else /* not DOS */
	    buf1 = xmalloc ((int) size);
	    buf2 = xmalloc ((int) size);
	    r1 = read (fd1, buf1, (int) size);
            if (r1 == -1)
		error (1, errno, "cannot read file %s for comparing", file1);
	    r2 = read (fd2, buf2, (int) size);
            if (r2 == -1)
		error (1, errno, "cannot read file %s for comparing", file2);
            if (r1 != r2)
                ret = 1;
            else
                ret = bcmp (buf1, buf2, r1);
	    free (buf1);
	    free (buf2);
#endif
	}
    }
    else
	ret = 1;
    (void) close (fd1);
    (void) close (fd2);
    return (ret);
}

/*
 * Recover the space allocated by Find_Names() and line2argv()
 */
void
free_names (pargc, argv)
    int *pargc;
    char *argv[];
{
    register int i;

    for (i = 0; i < *pargc; i++)
    {					/* only do through *pargc */
	free (argv[i]);
    }
    *pargc = 0;				/* and set it to zero when done */
}

/*
 * Convert a line into argc/argv components and return the result in the
 * arguments as passed.  Use free_names() to return the memory allocated here
 * back to the free pool.
 */
void
line2argv (pargc, argv, line)
    int *pargc;
    char *argv[];
    char *line;
{
    char *cp;

    *pargc = 0;
    for (cp = strtok (line, " \t"); cp; cp = strtok ((char *) NULL, " \t"))
    {
	argv[*pargc] = xstrdup (cp);
	(*pargc)++;
    }
}

/*
 * Returns the number of dots ('.') found in an RCS revision number
 */
int
numdots (s)
    char *s;
{
    char *cp;
    int dots = 0;

    for (cp = s; *cp; cp++)
    {
	if (*cp == '.')
	    dots++;
    }
    return (dots);
}

/*
 * Get the caller's login from his uid. If the real uid is "root" try LOGNAME
 * USER or getlogin(). If getlogin() and getpwuid() both fail, return
 * the uid as a string.
 */
char *
getcaller ()
{

#ifndef DOS
    static char uidname[20];
    struct passwd *pw;
    int uid;
#endif
    char *name;

#ifdef DOS
    if (((name = getenv("LOGNAME")) || (name = getenv("USER"))) && *name)
	return (name);
    else
	return ("UNKOWN");
#else
    uid = getuid ();
    if (uid == 0)
    {
	/* super-user; try getlogin() to distinguish */
	if (((name = getenv("LOGNAME")) || (name = getenv("USER")) ||
	     (name = getlogin ())) && *name)
	    return (name);
    }
    if ((pw = (struct passwd *) getpwuid (uid)) == NULL)
    {
	(void) sprintf (uidname, "uid%d", uid);
	return (uidname);
    }
    return (pw->pw_name);
#endif
}

/*
 * To exec a program under CVS, first call run_setup() to setup any initial
 * arguments.  The options to run_setup are essentially like printf(). The
 * arguments will be parsed into whitespace separated words and added to the
 * global run_argv list.
 * 
 * Then, optionally call run_arg() for each additional argument that you'd like
 * to pass to the executed program.
 * 
 * Finally, call run_exec() to execute the program with the specified arguments.
 * The execvp() syscall will be used, so that the PATH is searched correctly.
 * File redirections can be performed in the call to run_exec().
 */
static char *run_prog;
static char **run_argv;
static int run_argc;
static int run_argc_allocated;

/* VARARGS */
#if !defined (VPRINTF_MISSING) && __STDC__
void 
run_setup (char *fmt,...)
#else
void 
run_setup (fmt, va_alist)
    char *fmt;
    va_dcl

#endif
{
#ifndef VPRINTF_MISSING
    va_list args;

#endif
    char *cp;
    int i;

    run_init_prog ();

    /* clean out any malloc'ed values from run_argv */
    for (i = 0; i < run_argc; i++)
    {
	if (run_argv[i])
	{
	    free (run_argv[i]);
	    run_argv[i] = (char *) 0;
	}
    }
    run_argc = 0;

    /* process the varargs into run_prog */
#ifndef VPRINTF_MISSING
    VA_START (args, fmt);
    (void) vsprintf (run_prog, fmt, args);
    va_end (args);
#else
    (void) sprintf (run_prog, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
#endif

    /* put each word into run_argv, allocating it as we go */
    for (cp = strtok (run_prog, " \t"); cp; cp = strtok ((char *) NULL, " \t"))
	run_add_arg (cp);
}

void
run_arg (s)
    char *s;
{
    run_add_arg (s);
}

/* VARARGS */
#if !defined (VPRINTF_MISSING) && __STDC__
void 
run_args (char *fmt,...)
#else
void 
run_args (fmt, va_alist)
    char *fmt;
    va_dcl

#endif
{
#ifndef VPRINTF_MISSING
    va_list args;

#endif

    run_init_prog ();

    /* process the varargs into run_prog */
#ifndef VPRINTF_MISSING
    VA_START (args, fmt);
    (void) vsprintf (run_prog, fmt, args);
    va_end (args);
#else
    (void) sprintf (run_prog, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
#endif

    /* and add the (single) argument to the run_argv list */
    run_add_arg (run_prog);
}

static void
run_add_arg (s)
    char *s;
{
    /* allocate more argv entries if we've run out */
    if (run_argc >= run_argc_allocated)
    {
	run_argc_allocated += 50;
	run_argv = (char **) xrealloc ((char *) run_argv,
				     run_argc_allocated * sizeof (char **));
    }

    if (s)
	run_argv[run_argc++] = xstrdup (s);
    else
	run_argv[run_argc] = (char *) 0;/* not post-incremented on purpose! */
}

static void
run_init_prog ()
{
    /* make sure that run_prog is allocated once */
    if (run_prog == (char *) 0)
	run_prog = xmalloc (10 * 1024);	/* 10K of args for _setup and _arg */
}

int
run_exec (stin, stout, sterr, flags)
    char *stin;
    char *stout;
    char *sterr;
    int flags;
{
#if !defined(OS2) && !defined(DOS)	/* not OS/2, standard UNIX handling */
    int shin, shout, sherr;
    int mode_out, mode_err;
    int status = -1;
    int rerrno = 0;
    int pid, w;



#ifdef POSIX
    sigset_t sigset_mask, sigset_omask;
    struct sigaction act, iact, qact;

#else
#ifdef BSD_SIGNALS
    int mask;
    struct sigvec vec, ivec, qvec;

#else
    SIGTYPE (*istat) (), (*qstat) ();
#endif
#endif

    if (trace)
    {
	(void) fprintf (stderr, "-> system(");
	run_print (stderr);
	(void) fprintf (stderr, ")\n");
    }
    if (noexec && (flags & RUN_REALLY) == 0)
	return (0);

    /* make sure that we are null terminated, since we didn't calloc */
    run_add_arg ((char *) 0);

    /* setup default file descriptor numbers */
    shin = 0;
    shout = 1;
    sherr = 2;

    /* set the file modes for stdout and stderr */
    mode_out = mode_err = O_WRONLY | O_CREAT;
    mode_out |= ((flags & RUN_STDOUT_APPEND) ? O_APPEND : O_TRUNC);
    mode_err |= ((flags & RUN_STDERR_APPEND) ? O_APPEND : O_TRUNC);

    if (stin && (shin = open (stin, O_RDONLY)) == -1)
    {
	rerrno = errno;
	error (0, errno, "cannot open %s for reading (prog %s)",
	       stin, run_argv[0]);
	goto out0;
    }
    if (stout && (shout = open (stout, mode_out, 0666)) == -1)
    {
	rerrno = errno;
	error (0, errno, "cannot open %s for writing (prog %s)",
	       stout, run_argv[0]);
	goto out1;
    }
    if (sterr && (flags & RUN_COMBINED) == 0)
    {
	if ((sherr = open (sterr, mode_err, 0666)) == -1)
	{
	    rerrno = errno;
	    error (0, errno, "cannot open %s for writing (prog %s)",
		   sterr, run_argv[0]);
	    goto out2;
	}
    }

    /* The output files, if any, are now created.  Do the fork and dups */
#ifdef VFORK_MISSING
    pid = fork ();
#else
    pid = vfork ();
#endif
    if (pid == 0)
    {
	if (shin != 0)
	{
	    (void) dup2 (shin, 0);
	    (void) close (shin);
	}
	if (shout != 1)
	{
	    (void) dup2 (shout, 1);
	    (void) close (shout);
	}
	if (flags & RUN_COMBINED)
	    (void) dup2 (1, 2);
	else if (sherr != 2)
	{
	    (void) dup2 (sherr, 2);
	    (void) close (sherr);
	}

	/* dup'ing is done.  try to run it now */
	(void) execvp (run_argv[0], run_argv);
	_exit (127);
    }
    else if (pid == -1)
    {
	rerrno = errno;
	goto out;
    }
    /* the parent.  Ignore some signals for now */
#ifdef POSIX
    if (flags & RUN_SIGIGNORE)
    {
	act.sa_handler = SIG_IGN;
	(void) sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	(void) sigaction (SIGINT, &act, &iact);
	(void) sigaction (SIGQUIT, &act, &qact);
    }
    else
    {
	(void) sigemptyset (&sigset_mask);
	(void) sigaddset (&sigset_mask, SIGINT);
	(void) sigaddset (&sigset_mask, SIGQUIT);
	(void) sigprocmask (SIG_SETMASK, &sigset_mask, &sigset_omask);
    }
#else
#ifdef BSD_SIGNALS
    if (flags & RUN_SIGIGNORE)
    {
	bzero ((char *) &vec, sizeof (vec));
	vec.sv_handler = SIG_IGN;
	(void) sigvec (SIGINT, &vec, &ivec);
	(void) sigvec (SIGQUIT, &vec, &qvec);
    }
    else
	mask = sigblock (sigmask (SIGINT) | sigmask (SIGQUIT));
#else
    istat = signal (SIGINT, SIG_IGN);
    qstat = signal (SIGQUIT, SIG_IGN);
#endif
#endif

    /* wait for our process to die and munge return status */
#ifdef POSIX
    while ((w = waitpid (pid, &status, 0)) == -1 && errno == EINTR)
	;
#else
    while ((w = wait (&status)) != pid)
    {
	if (w == -1 && errno != EINTR)
	    break;
    }
#endif

    if (w == -1)
    {
	status = -1;
	rerrno = errno;
    }
    else if (WIFEXITED (status))
	status = WEXITSTATUS (status);
    else if (WIFSIGNALED (status))
    {
	if (WTERMSIG (status) == SIGPIPE)
	    error (1, 0, "broken pipe");
	status = 2;
    }
    else
	status = 1;

    /* restore the signals */
#ifdef POSIX
    if (flags & RUN_SIGIGNORE)
    {
	(void) sigaction (SIGINT, &iact, (struct sigaction *) NULL);
	(void) sigaction (SIGQUIT, &qact, (struct sigaction *) NULL);
    }
    else
	(void) sigprocmask (SIG_SETMASK, &sigset_omask, (sigset_t *) NULL);
#else
#ifdef BSD_SIGNALS
    if (flags & RUN_SIGIGNORE)
    {
	(void) sigvec (SIGINT, &ivec, (struct sigvec *) NULL);
	(void) sigvec (SIGQUIT, &qvec, (struct sigvec *) NULL);
    }
    else
	(void) sigsetmask (mask);
#else
    (void) signal (SIGINT, istat);
    (void) signal (SIGQUIT, qstat);
#endif
#endif

    /* cleanup the open file descriptors */
  out:
    if (sterr)
	(void) close (sherr);
  out2:
    if (stout)
	(void) close (shout);
  out1:
    if (stin)
	(void) close (shin);

  out0:
    if (rerrno)
	errno = rerrno;
    return (status);

#else  /* OS2 */
    int shin, shout, sherr;
    int sain, saout, saerr;	/* saved handles */
    int mode_out, mode_err;
    int status = -1;
    int rerrno = 0;
    int rval   = -1;
    int pid, w;

    if (trace)			/* if in trace mode */
    {
	(void) fprintf (stderr, "-> system(");
	run_print (stderr);
	(void) fprintf (stderr, ")\n");
    }
    if (noexec && (flags & RUN_REALLY) == 0) /* if in noexec mode */
	return (0);

    /*
     * start the engine and take off
     */

    /* make sure that we are null terminated, since we didn't calloc */
    run_add_arg ((char *) 0);

    /* setup default file descriptor numbers */
    shin = 0;
    shout = 1;
    sherr = 2;

    /* set the file modes for stdout and stderr */
    mode_out = mode_err = O_WRONLY | O_CREAT;
    mode_out |= ((flags & RUN_STDOUT_APPEND) ? O_APPEND : O_TRUNC);
    mode_err |= ((flags & RUN_STDERR_APPEND) ? O_APPEND : O_TRUNC);

    /* open the files as required, shXX are shadows of stdin... */
    if (stin && (shin = open (stin, O_RDONLY)) == -1)
    {
	rerrno = errno;
	error (0, errno, "cannot open %s for reading (prog %s)",
	       stin, run_argv[0]);
	goto out0;
    }
    if (stout && (shout = open (stout, mode_out, 0666)) == -1)
    {
	rerrno = errno;
	error (0, errno, "cannot open %s for writing (prog %s)",
	       stout, run_argv[0]);
	goto out1;
    }
    if (sterr && (flags & RUN_COMBINED) == 0)
    {
	if ((sherr = open (sterr, mode_err, 0666)) == -1)
	{
	    rerrno = errno;
	    error (0, errno, "cannot open %s for writing (prog %s)",
		   sterr, run_argv[0]);
	    goto out2;
	}
    }
    /* now save the standard handles, if required */
    sain = saout = saerr = -1;
    if (shin  != 0) sain  = dup( 0); /* dup stdin  */
    if (shout != 1) saout = dup( 1); /* dup stdout */
    if (sherr != 2) saerr = dup( 2); /* dup stderr */

    /* the new handles will be dup'd to the standard handles
     * for the spawn.
     */

    if (shin != 0)
      {
	(void) dup2 (shin, 0);
	(void) close (shin);
      }
    if (shout != 1)
      {
	(void) dup2 (shout, 1);
	(void) close (shout);
      }
    if (flags & RUN_COMBINED)
      (void) dup2 (1, 2);
    else if (sherr != 2)
      {
	(void) dup2 (sherr, 2);
	(void) close (sherr);
      }

    /* dup'ing is done.  try to run it now */
#ifdef DOS
    rval = spawnvpq ( P_WAIT, run_argv[0], run_argv);
#else
    rval = spawnvp ( P_WAIT, run_argv[0], run_argv);
#endif
    /* restore the original file handles   */
    if (sain  != -1) {
      (void) dup2( sain, 0);	/* re-connect stdin  */
      (void) close( sain);
    }
    if (saout != -1) {
      (void) dup2( saout, 1);	/* re-connect stdout */
      (void) close( saout);
    }
    if (saerr != -1) {
      (void) dup2( saerr, 2);	/* re-connect stderr */
      (void) close( saerr);
    }
    return rval;		/* end, if all went coorect */

    /* error cases */
    /* cleanup the open file descriptors */
  out:
    if (sterr)
	(void) close (sherr);
  out2:
    if (stout)
	(void) close (shout);
  out1:
    if (stin)
	(void) close (shin);

  out0:
    if (rerrno)
	errno = rerrno;
    return (status);
#endif /* OS2 */

}

void
run_print (fp)
    FILE *fp;
{
    int i;

    for (i = 0; i < run_argc; i++)
    {
	(void) fprintf (fp, "'%s'", run_argv[i]);
	if (i != run_argc - 1)
	    (void) fprintf (fp, " ");
    }
}

FILE *
Popen (cmd, mode)
    char *cmd, *mode;
{
    if (trace)
	(void) fprintf (stderr, "-> Popen(%s,%s)\n", cmd, mode);
    if (noexec)
	return (NULL);
    return (popen (cmd, mode));
}

#ifdef lint
#ifndef __GNUC__
/* ARGSUSED */
time_t
get_date (date, now)
    char *date;
    struct timeb *now;
{
    time_t foo = 0;

    return (foo);
}
#endif
#endif


#if defined(OS2) || defined(DOS)
char *index_sep(char *path)
{
  char *p1 = strchr(path, '\\');
  char *p2 = strchr(path, '/');
  if ( !p1 ) return p2;
  if ( !p2 ) return p1;
  return (p1 > p2) ? p2 : p1;
}


char *rindex_sep(char *path)
{
  char *p1 = strrchr(path, '\\');
  char *p2 = strrchr(path, '/');
  if ( !p1 ) return p2;
  if ( !p2 ) return p1;
  return (p1 > p2) ? p1 : p2;
}


#ifndef DOS
int _chmod( char *path, int mode)
{
  if( trace) fprintf( stderr, "-> _chmod( %s, %o)\n", path, mode);
#undef chmod
  return chmod( path, mode);
#define chmod( x, y) _chmod( x,y)
}
#endif

#endif /* OS/2 */

