# Makefile for minig directory

          AR = ar rv
          CC = cc
          RM = rm -f
      RM_CMD = $(RM) *.o core *~
   LIB_PATHS = -L. -L/usr/X11/lib
        LIBS = -lX11 -lm

EXE     = minig_test
ARCHIVE = ../../minig.a
OBJS    = set_focus.o miniga.o minigb.o minigc.o report_curs.o
#          minig_fortran.o

.c.o:
	$(RM) $@
	$(CC) $(CFLAGS) -c $*.c

all: $(ARCHIVE)

$(EXE): $(OBJS) minig_test.c
	$(CC) $(CFLAGS) -o $@ $(OBJS) minig_test.c $(LIB_PATHS) $(LIBS) ../../util.a -lreadline -lncurses `gtk-config --cflags --libs`

$(ARCHIVE): $(OBJS)
	$(RM) $@
	$(AR) $@ $(OBJS)
	-ranlib $@
# the - tells make to ignore an error.
# If running ranlib fails, there is probably not a ranlib for this system.
# so don't stop the build.

FORCE: 

clean: 
	$(RM_CMD)
very-clean:
	$(RM_CMD) $(ARCHIVE) $(EXE)
