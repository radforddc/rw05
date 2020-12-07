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

  /* get-event routine for GammaSphere early implementation data
     with Ge times */

  static int   evptr = -1, badevnt = 0, mult = 0, vsn = 0, oldvsn = 0;
  static int   words = 0, expectedwords = 0;
  static short buf[4096];


  if(evptr == -1)
    /* we have a new buffer */
    memcpy(buf, tbuf, gd.RecordSize);
  else {
    /* set up for a new event */
    badevnt = 0;
    mult = 0;
    vsn = 0;
    oldvsn = 0;
    words = 0;
    expectedwords = 0;
  }

  while (1) {
    /* loop forward in the event buffer */
    evptr++;
    if (evptr >= gd.RecordSize/2) {
      /* at the end of the buffer -- return -1 to get next one */
      evptr = -1;
      return -1;
    }
    /* printf(" evptr, data: %d %d %x\n",evptr,buf[evptr],buf[evptr]); */

    if (buf[evptr] == -256)
      /* have found end of event */
      break;

    if (buf[evptr] < 0) {
      /* word is a virtual station number */
      vsn = (buf[evptr])&2047;
      if (words != expectedwords) {
	/* if (!badevnt) printf("Bad event! %d words,  %d expected...\n",
         *                      words, expectedwords);
	 */
        badevnt = 1;
      }
      else if (vsn <= oldvsn) {
        /* vsn is supposed to increase */
	/* if (!badevnt) printf("Bad event! VSN %d <= %d...\n", vsn, oldvsn); */
	badevnt = 1;
      }
      oldvsn = vsn;
      words = 0;
      expectedwords = ((buf[evptr]>>11))&15;

      if (vsn > 9) {
        /* data will be for Ge times */
	if (expectedwords == 0)
	  expectedwords = 16;
      }
      else {
        /* data will be for Ge energies */
	if (expectedwords > 3) {
	  /* if (!badevnt) printf("Bad event! VSN: %d, expectedwords: %d...\n",
           *                      vsn, expectedwords);
           */
	  badevnt = 1;
	}
      }
      continue;
    }

    /* if we got here, we have a data word */
    words++;
    if (mult >= RW_MAXMULT)
      continue;
    if (vsn > 9)
      continue;   /* data is for Ge time */

    elist[mult] = (buf[evptr])&8191;
    mult++;
  }

  /* printf("mult: %d  e: %d %d %d %d %d\n",
   *       mult, elist[0], elist[1], elist[2], elist[3], elist[4]);
   */

  if (badevnt)
    return 0;
  return mult;
}
