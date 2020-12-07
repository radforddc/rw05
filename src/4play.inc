Er154,155 4d tests  Eurogam presorted tapes
/dev/rmt0h
er.scr
er.4d
er.tab
16384
1500
n

--------------------------------------------------
The above lines provide the following information to 4play:
line 1:  user-provided title for the replay task
line 2:  name for the input tape drive device to be used
line 3:  name to be used for the "scratch" disk file
line 4:  name of the 4D hypercube directory file
line 5:  name of the look-up table file to be used to map ADC chs to cube chs
         (generally produced by running the program lufwhm)
line 6:  block size for the records to be read from tape, in bytes
line 7:  size to be used for the "scratch" disk file, in MB
         (should generally be at least several hundred MB
          in order to minimize scanning time)
line 8:  y(es) or n(o) to specify whether the records read from tape need
         to be byte-swapped

NOTE:    If the 4D hypercube directory file does not yet exist (i.e. you
         are starting a new replay) you will need to create it by running
         the program make4cub.  Recommended filename extensions are .4d
         for the directory file and .4cub for the actual hypercube files.
