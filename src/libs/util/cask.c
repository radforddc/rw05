#include <stdio.h>
#ifndef VMS
#include <termios.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "util.h"

#ifdef HAVE_GNU_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

FILE *infile = 0, *cffile = 0;    /* command file flag and file descriptor*/
int  cflog = 0;                   /* command logging flag */

/* ======================================================================= */
int read_cmd_file(char *outstring, int maxretlen)
{
  /* return -1 if command file is not open for reading */
  if (!infile) return -1;
  /* otherwise read a line from the file and return the # of chars */
  return cask("?", outstring, maxretlen);
} /* read_cmd_file */

/* ======================================================================= */
int log_to_cmd_file(char *string)
{
  /* return 1 if command file is not open for logging */
  if (!cflog || !cffile) return 1;
  /* otherwise write string to the file */
  fprintf(cffile, "%s\n", string);
  return 0;
} /* log_to_cmd_file */

/* ======================================================================= */
int cask(char *mesag, char *ans, int mca)
{
  /* mesag:  question to be asked, should be less than 4096 chars
     ans:    answer recieved, maximum of mca chars will be modified
     mca:    max. number of characters asked for in answer
     returns number of characters received in answer */

  FILE *file;

#ifdef XWIN
  /* modified for use with minig */
  extern int get_focus(int *);
  extern int set_focus(int);
  static int focus = 0;
#endif

#ifdef VMS
  extern char getchar_vms(int echoflag);
#else
  struct termios s, t;
#endif
  int    nca, i;
  char   *wfr, *prompt, nope = '\0';
#ifdef HAVE_GNU_READLINE
  char   *inputline;
#endif
  static char r[4096];

  if (!(prompt = getenv("RADWARE_PROMPT"))) prompt = &nope;

  if (mca == 0) {
    tell("%s", mesag); /* write message */
    return 0;
  }

#ifdef XWIN
  /* set input focus to original window */
  if (focus && !infile) set_focus(focus);
#endif

  if (infile) {
    /* write message */
    tell("%s%s", mesag, prompt);
    /* read response string from input command file */
    if (!fgets(r, 256, infile)) {
      /* have reached end of command file */
      infile = 0;
      strncpy(ans, "CF END", mca);
      nca = 6;
      return nca;
    }
    /* if reading from a file, and we read a line "?",
       then go to interactive mode to get a real answer from the user */
    if (!strcmp(r, "?") || !strcmp(r, "?\n") || !strcmp(r, "?\r\n")) {
      tell("\n");
      file = infile;
      infile = 0;
      nca = cask(mesag, ans, mca);
      infile = file;
    }
  } else if (mca > 4                           /* will accept a long string */
	     || ((wfr = getenv("RADWARE_AWAIT_RETURN")) && 
		 (*wfr == 'Y' || *wfr == 'y')) /* or wants to wait for \n */
#ifndef VMS
	     || (tcgetattr(0, &s))             /* or cannot get term attr */
#endif
	     ) {
    /* read string from stdin */
#ifdef HAVE_GNU_READLINE
    if (strlen(mesag) + strlen(prompt) > 4095) {
      strncpy(r, mesag, 4095);
    } else{
      snprintf(r, 4095, "%s%s", mesag, prompt); /* save mesag + prompt in r */
    }
    inputline = readline(r); /* use readline write r and to read response */
    strncpy(r, inputline, 256);
    if (strlen(inputline)) add_history(inputline);
    free(inputline);
#else
    /* write message */
    tell("%s%s", mesag, prompt);
    /* read response string from input command file */
    fgets(r, 256, stdin);
#endif
  } else {
    /* write message */
    tell("%s%s", mesag, prompt);
    /* read chars one-at-a-time from stdin */
#ifdef VMS
    i = 0;
    while ((r[i++] = getchar_vms(1)) != '\n' && i < mca);
#else
    tcgetattr(0, &s);
    tcgetattr(0, &t);
    t.c_lflag &= ~ICANON;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
    i = 0;
    while ((r[i++] = (char) getchar()) != '\n' && i < mca);
    tcsetattr(0, TCSANOW, &s);
#endif
    if (r[i-1] != '\n') tell("\n");
    r[i] = '\0';
  }
  if ((nca = strlen(r)) > mca) {
    nca = mca;
    r[mca] = '\0';
  }

#ifdef XWIN
  /* save information about current input focus window */
  if (!focus && !infile) get_focus(&focus);
#endif

  /* remove trailing blanks, \r or \n */
  while (nca > 0 &&
	 (r[nca-1] == ' ' || r[nca-1] == '\n' || r[nca-1] == '\r')) {
    r[--nca] = '\0';
  }

  /* if reading from command file, echo response to stdout */
  if (infile) tell("%s\n", r);
  /* if log command file open, copy response */
  if (cflog) fprintf(cffile, "%s\n", r);

  /* copy response to ans */
  strncpy(ans, r, mca);
  return nca;
} /* cask */
