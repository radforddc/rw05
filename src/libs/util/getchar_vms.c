#include <stdio.h>
#include <iodef.h>
#include <descrip.h>
#include <ssdef.h>

int assign_ioc(int *iochan_out)
{
  /* assign i/o channel to terminal for qio calls */

  extern int sys$assign(), lib$signal();
  static int iochan = 0;
  static $DESCRIPTOR(device, "tt:");
  int iok;

  if (iochan == 0) {
    iok =sys$assign(&device,&iochan,0,0,0);
    if (iok != SS$_NORMAL) {
      printf("ERROR - cannot assign i/o channel!\n");
      lib$signal(iok);
      *iochan_out = 0;
      return 1;
    }
  }
  *iochan_out = iochan;
  return 0;
}

char getchar_vms(int echoflag)
{

  static int iochan = 0, eventflag;
  extern int sys$qiow(), lib$signal(), lib$get_ef();
  int  ioflag, iok;
  char ans[16];

  struct {
    short iostat, term_offset, terminator, term_size;
  } iosb;


  /* assign i/o channel to terminal on first entry */
  if (iochan == 0) {
    if (assign_ioc(&iochan)) return 0;
    iok = lib$get_ef(&eventflag);
    if (iok != SS$_NORMAL) eventflag = 0;
  }

  if (echoflag) {
    ioflag = IO$_READVBLK;
  } else {
    ioflag = IO$M_NOECHO | IO$_READVBLK | IO$M_TIMED;
  }
  /* printf("ioflag, iochan: %d %d\n", ioflag, iochan); */
  iok = sys$qiow(eventflag,iochan,ioflag,&iosb,0,0,&ans,1,0,0,0,0);
  if (iok != SS$_NORMAL) {
    lib$signal(iok);
    return 0;
  } else if (iosb.iostat != SS$_NORMAL) {
    if (echoflag) lib$signal(iosb.iostat);
    return 0;
  }

  return *ans;
}
