# Makefile for util directory

          AR = ar rv
          CC = cc
          RM = rm -f
      RM_CMD = $(RM) *.o core *~
   LIB_PATHS = -L. -L/usr/X11/lib
        LIBS = -lX11 -lm

ARCHIVE = ../../util.a
OBJS    = cask.o caskyn.o ask.o matinv.o matread.o readsp.o rwspec.o util.o

ifdef HISLIBS
readsp.o: readsp.c util.h
	$(RM) $@
	$(CC) $(CFLAGS) -DHISFILES -c readsp.c
endif

.c.o:
	$(RM) $@
	$(CC) $(CFLAGS) -c $*.c

all: $(ARCHIVE)

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
	$(RM_CMD) $(ARCHIVE)
