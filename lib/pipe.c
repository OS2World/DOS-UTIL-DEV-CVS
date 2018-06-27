#include "system.h"
#include <stdio.h>

#define _NFILE	10

typedef enum { unopened = 0, reading, writing } pipemode;

static struct
{
    char *name;
    char *command;
    pipemode pmode;
}
pipes[_NFILE];

static FILE *dos_popen(char *command, char *mode)
{
    FILE *current;
    char name[PATH_MAX];
    char *tmp = getenv("TMP");
    int cur;
    pipemode curmode;

    /*
    ** decide on mode.
    */
    if(strchr(mode, 'r') != NULL)
        curmode = reading;
    else if(strchr(mode, 'w') != NULL)
        curmode = writing;
    else
        return NULL;

    /*
    ** get a name to use.
    */

    strcpy(name, tmp ? tmp : "\\");
    if ( name[strlen(name) - 1] != '\\' )
      strcat(name, "\\");
    strcat(name, "piXXXXXX");
    mktemp(name);

    /*
    ** If we're reading, just call system to get a file filled with
    ** output.
    */
    if(curmode == reading)
    {
        char cmd[256];
        sprintf(cmd,"%s > %s", command, name);
        system(cmd);

        if((current = fopen(name, mode)) == NULL)
	    return NULL;
    }
    else
    {
        if((current = fopen(name, mode)) == NULL)
            return NULL;
    }

    cur = fileno(current);
    pipes[cur].name = strdup(name);
    pipes[cur].command = strdup(command);
    pipes[cur].pmode = curmode;

    return current;
}

static int dos_pclose(FILE * current)
{
    int cur = fileno(current), rval;
    char command[256];

    /*
    ** check for an open file.
    */
    if(pipes[cur].pmode == unopened)
        return -1;

    if(pipes[cur].pmode == reading)
    {
        /*
        ** input pipes are just files we're done with.
        */
        rval = fclose(current);
        unlink(pipes[cur].name);
    }
    else
    {
        /*
        ** output pipes are temporary files we have
	** to cram down the throats of programs.
        */
	fclose(current);
        sprintf(command,"%s < %s", pipes[cur].command, pipes[cur].name);
        rval = system(command);
        unlink(pipes[cur].name);
    }

    /*
    ** clean up current pipe.
    */
    free(pipes[cur].name);
    free(pipes[cur].command);
    pipes[cur].pmode = unopened;

    return rval;
}


FILE *popen(char *cmd, char *mode)
{
#ifdef __TURBOC__
  return (dos_popen(cmd, mode));
#else
  return (_osmode == DOS_MODE) ? dos_popen(cmd, mode) : _popen(cmd, mode);
#endif
}

int pclose(FILE *ptr)
{
#ifdef __TURBOC__
  return (dos_pclose(ptr));
#else
  return (_osmode == DOS_MODE) ? dos_pclose(ptr) : _pclose(ptr);
#endif
}
