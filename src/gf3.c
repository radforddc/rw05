/* version 0.0   D.C. Radford    July 1999 */

#include <stdio.h>
#include <string.h>

int main (int argc, char **argv)
{
  extern int cask(char *, char *, int);
  extern int gfexec(char *, int);
  extern int gfinit(int argc, char **argv);
  int  i, nc;
  char ans[80], next[80], *c;

  /* welcome and get initial data */
  gfinit(argc, argv);

  while (1) {
    /* decode and execute commands */
    if ((nc = cask("?", ans, 80)) > 1) {
      // use ; to put multiple commands on one line
      while (strncmp(ans, "SY", 2) && strncmp(ans, "sy", 2) &&
             (c = strstr(ans, ";")) && strlen(c) > 2) {
        strncpy(next, c+1, sizeof(next));
        *c = '\0';
        gfexec(ans, strlen(ans));
        for (i = 0; next[i] == ' '; i++) ; // remove leading spaces
        strncpy(ans, next+i, sizeof(ans));
        nc = strlen(ans);
      }
      gfexec(ans, nc);
    }
  }
} /* main gf3 */
