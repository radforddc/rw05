# Makefile for minig directory

          AR = ar rv
          CC = cc
          RM = rm -f
      RM_CMD = $(RM) *.o core *~
   LIB_PATHS = -L. -L/usr/X11/lib
        LIBS = -lX11 -lm
      CFLAGS = -Wall

EXE     = mingtk_test
ARCHIVE = ../../mingtk.a
OBJS    = set_focus.o miniga.o minigb.o minigc.o report_curs.o

ifdef GTK2
         GTKFLAGS = `pkg-config --cflags gtk+-2.0`
         GTKLIBS  = `pkg-config --cflags --libs gtk+-2.0`
else
         GTKFLAGS = `gtk-config --cflags`
         GTKLIBS  = `gtk-config --cflags --libs`
endif

.c.o:
	$(RM) $@
	$(CC) $(CFLAGS) -c $*.c $(GTKFLAGS)

all: $(ARCHIVE)

$(EXE): $(OBJS) mingtk_test.c
	$(CC) $(CFLAGS) -o $@ $(OBJS) mingtk_test.c $(LIB_PATHS) $(LIBS) ../../util.a -lreadline -lncurses $(GTKLIBS)

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
