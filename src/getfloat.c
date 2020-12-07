#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

/*========================================================================*/
int base(int *i, char *in) {

/* This subroutine finds the start of a string (i.e remove initial blanks).
   It returns 1 if it went past end of line.
   A '!' character is treated as an end of line marker,
     so that comments may be made on any line after this character. */

  while ( *i < 119 && in[*i] == ' ') ++(*i);
  if (*i > 118 ||
      in[*i] == '\n' || in[*i] == '\0' ||
      in[*i] == '&' || in[*i] == '!') return 1;
  return 0;
} /* base */

/* ================================================================ */
int get(int *i, char *in, float *result, int line)
{
  float temp;
  int   len = 0;

/* This subroutine reads in float numbers from the string in,
   starting from character i.
   Returns 1 if there are no more characters on the line.
   Returns 2 if two commas are encountered in a row.
   Returns -1, and outputs a message, if a numerical error is encountered.
   If returned value is 0, the number is returned as result.
   Otherwise, result is left unchanged.
   The index pointer i is updated on exit. */

  /* find starting position */
  if (base(i, in)) return 1;

  /* if first character is comma, then remove any blanks after it.
     if a new line was read, then assign default */
  if (in[*i] == ',') {
    ++(*i);
    if (base(i, in)) return 1;
  }

  /* find length of input field */
  while (in[*i + len] != ' ' &&
	 in[*i + len] != ',' &&
	 in[*i + len] != '\0' &&
	 in[*i + len] != '\n') ++len;
  if (len == 0) return 2;

  if (sscanf(in + *i, "%f", &temp) != 1) {
    printf("NUMERICAL ERROR, line %d\n %s\n", line, in);
    *i += len;
    return -1;
  }
  *result = temp;
  *i += len;
  return 0;
} /* get */
