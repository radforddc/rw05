#include <stdio.h>
#include "util.h"

/* ====================================================================== */
int caskyn(char *mesag)
{
  /* mesag:     question to be asked (character string)
     returns 1: answer = Y/y/1   0: answer = N/n/0/<return> */

  char ans[16];

  while (cask(mesag, ans, 1)) {
    if (ans[0] == 'N' || ans[0] == 'n' || ans[0] == '0') return 0;
    if (ans[0] == 'Y' || ans[0] == 'y' || ans[0] == '1') return 1;
  }
  return 0;
} /* caskyn */
