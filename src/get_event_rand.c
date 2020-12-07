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
  static float d = (float) (RAND_MAX) + 1.0;
  static int   save=0;
  float  c1, c2;
  int    ret, i;

  /*  generates events of random energies, multiplicity = 9 */

  if (save == 400) {
    /* pretend we reached end of tape buffer after every 400 events */
    save = 0;
    return -1;
  }
  save++;

  ret = 9;
  for (i=0; i<ret; i++) {
    /* generate nine random energies between ch 80 and ch 330 */
    c1 = ((float) rand())/d;
    c2 = ((float) rand())/d;
    if (c1 < 0.5)
      /* 50% of time use flat distribution  */
      elist[i] = 1 + (int)(940.0*c2);
    else {
      /* 50% of time use quadratic distribution  */
      c1 = ((float) rand())/d;
      elist[i] = 1 + (int)(940.0*c1*c2);
    }
  }
  return ret;
}
