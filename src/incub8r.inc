Er154,155  Eurogam2 presorted tapes
/dev/rmt0h
er.scr
er.cub
er.tab
16384
500
n

--------------------------------------------------
The above lines provide the following information to incub8r3:
line 1:  user-provided title for the replay task
line 2:  name for the input tape drive device to be used
line 3:  name to be used for the "scratch" disk file
line 4:  name of the 3D cube file
line 5:  name of the look-up table file to be used to map ADC chs to cube chs
         (generally produced by running the program lufwhm)
line 6:  block size for the records to be read from tape, in bytes
line 7:  size to be used for the "scratch" disk file, in MB
         (should generally be at least several hundred MB
          in order to minimize scanning time)
line 8:  y(es) or n(o) to specify whether the records read from tape need
         to be byte-swapped