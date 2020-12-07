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
     without Ge times */

  static int   evptr = -1,  mult = 0;
  static short buf[4096];


  if(evptr == -1)
    memcpy(buf, tbuf, gd.RecordSize);
  else
    mult = 0;

  while (1) {
    evptr++;
    if (evptr >= gd.RecordSize/2) {
      evptr = -1;
      return -1;
    }

    if (buf[evptr] == -256)
      break;

    if (buf[evptr] < 0)
      continue;

    if (mult >= RW_MAXMULT)
      continue;

    elist[mult] = buf[evptr];
    mult++;
  }

/* printf("mult: %d  e: %d %d %d %d %d\n",
         mult, elist[0], elist[1], elist[2], elist[3], elist[4]); */
  return mult;
}
