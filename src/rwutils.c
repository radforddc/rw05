/************************************************************
          Utility to return formatted date and time....
*************************************************************/
char *datim (void)
{
  time_t tm;

  tm = time(NULL);
  return (ctime(&tm));
}

/************************************************************
          Utility to set filename extensions....
  fn     = filename
  extIn  = desired extension
  force  = 0 to add extIn if no .ext exists,
           1 to replace any existing .ext with extIn
  returns (char *) pointer to fn extension after execution
*************************************************************/
char *setfnamext (char *fn, char *extIn, int force)
{
  char *extOut, *tmp, *c;

  /* remove leading spaces */
  while (fn[0] == ' ') for (c=fn; *c!=0; c++) *c = *(c+1);
  /* check for an extension */
  if ((tmp = strrchr(fn,'/')) ||
      (tmp = strrchr(fn,']')) ||
      (tmp = strrchr(fn,':'))) {
    extOut = strrchr(tmp,'.');
  } else {
    extOut = strrchr(fn,'.');
  }

  if (!extOut) {
    extOut = fn + strlen(fn);
    strcat(fn, extIn);
  } else if (force) {
    *extOut = 0;
    strcat(fn, extIn);
  }
  return (extOut);
}

/***************************************************************************
   read_tab: returns 0 on success or 1 on failure
     lnam       - lookup table file name on disk
     nclook     - returned length (in chs) of lookup table
     lmin, lmax - returned min, max values in lookup table
     lut_ret    - returned lookup table values
*/
int read_tab(char *lnam, int *nclook, int *lmin, int *lmax, short *lut_ret)
{
  int   ibuf;
#ifdef VMS
  int   j;
  short *buf;
#endif
  FILE  *file;

  if (!(file=fopen(lnam,"r"))) {
    printf("\007  ERROR -- Could not open %s for reading.\n",lnam);
    return 1;
  }
  fread(&ibuf,4,1,file); /* FORTRAN stuff */
#ifdef VMS
  buf = (short *) &ibuf;
  if (*buf != 14) {
#else
  if (ibuf != 12) {
#endif
    printf("\007  ERROR -- Bad record(1) in look-up file.\n");
    fclose(file);
    return 1;
  }
  if ( (fread(nclook,4,1,file)) < 1 ||  /* Number of chs in lookup table */
       (fread(lmin,  4,1,file)) < 1 ||  /* Minimum value in lookup table */
       (fread(lmax,  4,1,file)) < 1 ) { /* Maximum value in lookup table */
  ERR:
    printf("\007  ERROR -- Unexpected end of look-up file.\n");
    fclose(file);
    return 1;
  }
  if (*nclook > 16384) *nclook = 16384;
  fread(&ibuf,4,1,file); /* FORTRAN stuff */

#ifdef VMS
  j   = *nclook;
  buf = lut_ret;
  while (j > 2042) {
    if (fread(buf, 2042*2, 1, file) < 1) goto ERR;
    fread(&ibuf,4,1,file); /* FORTRAN stuff */
    buf += 2042; j -= 2042;
  }
  if (fread(buf, j*2, 1, file) < 1) goto ERR;
#else
  fread(&ibuf,4,1,file); /* FORTRAN stuff */
  if ((fread(lut_ret,(*nclook)*2,1,file)) < 1) goto ERR;
#endif
  fclose(file);
  return 0;
}
