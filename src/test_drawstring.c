#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "minig.h"

int drawstring(char *, int, char, float, float, float, float);
int drawstr_ps(char *, int, char, float, float, float, float);
int choose_font(int);
int closeps(void);
int openps(char *, char *, char *, float, float, float, float);

int main(void)
{
  float csx = 4.0f, csy = 4.8f, x, y;
  int   ncs, nsx, nsy;
  char  filnam[80], ostring[80];
  FILE  *file;

  /* test routine for subroutines in drawstring.c and drawstr_ps.c
     .cmd file has character strings to be drawn */

  initg(&nsx, &nsy);
  finig();
  limg(nsx, 0, nsy, 0);
  trax(200.0f, 0.0f, 150.0f, 0.0f, 0);

  while (cask("File = ? (default .ext = .cmd, rtn to exit)", filnam, 80)) {
    setext(filnam, ".cmd", 80);
    if (!(file = open_readonly(filnam))) continue;
    initg(&nsx, &nsy);
    erase();

    /* read, decode and output lines from file */
    x = 100.f;
    y = 150.f;
    while (fgets(ostring, 80, file)) {
      ncs = strlen(ostring);
      y -= 10.0f;
      drawstring(ostring, ncs, 'H', x, y, csx, csy);
    }
    finig();
    if (caskyn("Make .ps file? (Y/N)")) {
      /* open output file */
      openps("demo_drawstr.ps",
	     "test_drawstr_ps (Author: D.C. Radford)",
	     "Test of drawstr_ps routines",
	     0.0f, 200.0f, 0.0, 200.0f);
      rewind(file);
      x = 100.0f;
      y = 180.0f;
      while (fgets(ostring, 80, file)) {
	drawstr_ps(ostring, strlen(ostring), 'H', x, y, csx, csy);
	y -= 10.0f;
      }
      closeps();
    }
    fclose(file);
  }
  return 0;
} /* main */
