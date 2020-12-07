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
  static int w = 120, x = 130, y = 140, z = 150;

  /* generates events of energies that form a grid, multiplicity = 4 */
  /* channel numbers are 120, 130, 140, ... 270                      */
  /* energies are ordered and off-diagonal, i.e.   w < x < y < z     */

  if (z >= 280) {
    z = 150;
    return -1;  /* pretend we reached end of tape buffer and restart grid */
  }

  elist[0] = w;
  elist[1] = x;
  elist[2] = y;
  elist[3] = z;

  /* increment energies by 10 chs */
  if ((w += 10) >= x) {
    w = 120;
    if ((x += 10) >= y) {
      x = 130;
      if ((y += 10) >= z) {
	y = 140;
	z += 10;
      }
    }
  }

  return 4;
}
