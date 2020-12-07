#include <stdio.h>
#include <signal.h>
#include "util.h"
#include "levit8r.h"

int main(int argc, char **argv)
{
  int   nc;
  char  ans[80];

  l_gls_init(argc, argv);

  while (1) {
    if ((nc = cask("Next command?", ans, 80))) l_gls_exec(ans, nc);
  }
}

void breakhandler(int dummy)
{
  elgd.gotsignal = 1;
}

void set_signal_alert(int mode, char *mesag)
{

  if (mode) {
    tell("%s\nYou can press control-C to interrupt...\n", mesag);
    elgd.gotsignal = 0;
    signal(SIGINT, breakhandler);
  } else {
    signal(SIGINT, SIG_DFL);
  }
}

int check_signal_alert(void)
{
  return elgd.gotsignal;
}
