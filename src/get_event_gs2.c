/****************************************************************************
 *    get_event routine for 4play or incub8r3                               *
 *    get_event is called by the main program on an event-by-event basis    *
 *      to extract the Ge energies from the global tape data buffer         *
 *          char *tbuf    of size    gd.RecordSize.                         *
 *    The Ge energies are returned in the array                             *
 *          int *elist    of size    RW_MAXMULT.                            *
 *    The returned value from get_event is the number of Ge energies,       *
 *      i.e. the Ge multiplicity,                                           *
 *      or -1 if the end of the data buffer is reached.                     *
 *      Thus if get_event returns the value -1, the main program            *
 *      will read the next tape buffer before calling get_event again.      *
 ***************************************************************************/

int
get_event(int *elist)
{

  /* get-event routine for GammaSphere standard format */

  static int   evptr = -1, maxptr;
  static short buf[8192];
  int    mult, ncge, evlen, nextev;


  if (evptr == -1) {
    /* we have a new buffer */
    memcpy(buf, tbuf, gd.RecordSize);
    evptr = buf[3]/2;         /* skip over buffer header */
    maxptr = evptr + buf[8];  /* length of data in buffer */
  } else if (evptr >= maxptr) {
    /* we are at the end of the buffer -- return -1 to get next one */
    evptr = -1;
    return -1;
  }

  /* process event */
  evlen = buf[evptr]&8191;
  if (evlen == 0) {   /* corrupted data in buffer */
    evptr = -1;
    return -1;        /* return -1 to get next buffer */
  }
    
  nextev = evptr + evlen;
  if (buf[nextev-1] != -1) {          /* check for end-of-event */
    while (buf[evptr] != -1) evptr++;   /* find next end-of-event */
    evptr++;
    return 0;   /* return bad event */
  }
  ncge = buf[evptr+1];

  /* now skip down to clean Ge data and extract energies */
  evptr += 10;   /* 10 words in event header */
  for (mult = 0; mult < ncge; mult++) {
    elist[mult] = buf[evptr+1]&8191;
    evptr += 6;  /* 6 words per clean Ge */
  }

  /* printf("mult: %d  e: %d %d %d %d %d\n",
   *       mult, elist[0], elist[1], elist[2], elist[3], elist[4]);
   */

  evptr = nextev;
  return mult;
}
