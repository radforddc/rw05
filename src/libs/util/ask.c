#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "util.h"


/* ====================================================================== */
int askfn(char *ans, int mca,
	  const char *default_fn, const char *default_ext,
	  const char *fmt, ...)
{
  /* fmt, ...:  question to be asked
     ans:       answer recieved, maximum of mca chars will be modified
     mca:       max. number of characters asked for in answer
     returns number of characters received in answer */
  /* this variant of ask asks for filenames;
     default_fn  = default filename (or empty string if no default)
     default_ext = default filename extension (or empty string if no default) */

  va_list ap;
  char    q[4096];
  int     nca = 0, ncq, j;

  va_start(ap, fmt);
  ncq = vsnprintf(q, 4095, fmt, ap);
  va_end(ap);

  if (strlen(default_ext)) {
    j = snprintf(q+ncq, 4095-ncq, "\n   (default .ext = %s)", default_ext);
    ncq += j;
  }
  if (strlen(default_fn)) {
    snprintf(q+ncq, 4095-ncq, "\n   (rtn for %s)", default_fn);
  }
 
  nca = cask(q, ans, mca);
  if (strlen(default_fn) && nca == 0) {
    strncpy(ans, default_fn, mca);
    nca = strlen(default_fn);
  }
  setext(ans, default_ext, mca);

  return nca;
} /* askfn */

/* ====================================================================== */
int askyn(const char *fmt, ...)
{
  /* fmt, ...:  question to be asked (format string and variable no. of args)
     returns 1: answer = Y/y/1   0: answer = N/n/0/<return> */

  va_list ap;
  char    q[4096];

  va_start(ap, fmt);
  vsnprintf(q, 4095, fmt, ap);
  va_end(ap);

  return caskyn(q);
} /* askyn */

/* ====================================================================== */
int ask(char *ans, int mca, const char *fmt, ...)
{
  /* fmt, ...:  question to be asked
     ans:       answer recieved, maximum of mca chars will be modified
     mca:       max. number of characters asked for in answer
     returns number of characters received in answer */

  va_list ap;
  char    q[4096];

  va_start(ap, fmt);
  vsnprintf(q, 4095, fmt, ap);
  va_end(ap);
  return cask(q, ans, mca);
}

/* ====================================================================== */
void tell(const char *fmt, ...)
{
  /* fmt, ...:  string to be output (format string and variable no. of args) */

  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
} /* tell */

/* ====================================================================== */
void warn1(const char *fmt, ...)
{
  /* fmt, ...:  string to be output (format string and variable no. of args) */
  /* same as tell() for standard command-line programs
     but redefined elsewhere as a popup for GUI versions */

  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
} /* warn */

/* ====================================================================== */
void warn(const char *fmt, ...)
{
  /* fmt, ...:  string to be output (format string and variable no. of args) */

  va_list ap;
  char    ans[16];

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  (void) ask(ans, 1, "Press any key to continue...\n");
} /* warn */

/* ====================================================================== */
int ask_selection(int nsel, int default_sel, const char **choices,
		 const char *flags, const char *head, const char *tail)
{
  /* returns item number selected from among nsel choices (counting from 0)
     default_sel = default choice number (used for simple <rtn>, no choice)
     choices = array of strings from which to choose
     flags = optional array of chars to use as labels for the choices
     head, tail = strings at start and end of question
  */

  char q[4096], f[48], ans[8];
  int  nca = 0, ncq = 0, i;


  if (nsel < 2 || nsel > 36) return 0;

  if (*head) ncq = snprintf(q, 4095, "%s\n", head);

  if (strlen(flags) >= nsel) {
    strncpy(f, flags, nsel);
  } else {
    for (i = 0; i < 10; i++) f[i] = (char) (48 + i);
    for (i = 10; i < nsel; i++) f[i] = (char) (55 + i);
  }

  if (*head) ncq = snprintf(q+ncq, 4095, "%s\n", head);
  ncq += snprintf(q+ncq, 4095-ncq, "Select %c for %s,\n", f[0], choices[0]);
  for (i = 1; i < nsel-1; i++) {
   ncq += snprintf(q+ncq, 4095-ncq, "       %c for %s,\n", f[i], choices[i]);
  }
  ncq += snprintf(q+ncq, 4095-ncq, "    or %c for %s.\n",
		  f[nsel-1], choices[nsel-1]);
  if (default_sel >= 0 && default_sel <= nsel)
    ncq += snprintf(q+ncq, 4095-ncq,
		    "   (Default is %s)\n", choices[default_sel-1]);
  if (*tail)
    ncq += snprintf(q+ncq, 4095-ncq, "%s\n", tail);
  ncq += snprintf(q+ncq, 4095-ncq, "  ...Your choice = ?");
 
  while (1) {
    nca = ask(ans, 1, q);
    if (nca == 0) return default_sel;
    for (i = 0; i < nsel; i++) {
      if (*ans == f[i] ||
	  (f[i] >= 'A' && f[i] <= 'Z' && *ans == f[i] + 'a' - 'A') ||
	  (f[i] >= 'a' && f[i] <= 'z' && *ans == f[i] + 'A' - 'a'))
	return i;
    }
  }
} /* ask_selection */
