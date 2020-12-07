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

  /* get-event routine for Eurogam-II presorted tapes */

  static int   evptr = -1,  mult = 0;
  static short buf[8192 + 2];  /* 2 extra words for safety's sake */
  int  i;
  char tmp;


  if(evptr<0) {
        /* we have a new buffer */
    for (i=1; i<gd.RecordSize; i+=2) {
      tmp = tbuf[i];
      tbuf[i] = tbuf[i-1];
      tbuf[i-1] = tmp;
    }
    memcpy(buf, tbuf, gd.RecordSize);
    evptr = 13;
  }

  /* set up for a new event */
  mult=0;

  while (1) {
    /* loop forward in the event buffer */
    evptr++;
    if (evptr>=gd.RecordSize/2) {
      /* at the end of the buffer -- return -1 to get next one */
      evptr = -1;
      return -1;
    }
    /* printf(" evptr, data: %d %d %x\n",evptr,buf[evptr],buf[evptr]); */

    if (buf[evptr]==-1) {
      /* have found end of event */
      evptr++;
      break;
    }

    evptr++;
    if (mult<RW_MAXMULT)
      elist[mult++] = buf[evptr]&8191;
  }

  /* printf("mult: %d  e: %d %d %d %d %d\n",
   *       mult, elist[0], elist[1], elist[2], elist[3], elist[4]);
   */
  return mult;
}
