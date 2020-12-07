# Makefile for gls directory

          AR = ar rv
          CC = cc
          RM = rm -f
      RM_CMD = $(RM) *.o core *~
   LIB_PATHS = -L. -L/usr/X11/lib
        LIBS = -lX11 -lm

ARCHIVE = ../../gls.a
OBJS    = glsa.o glsb.o glsc.o glsd.o fitl.o hsicc.o

glsa.o: glsa.c gls.h
	$(RM) $@
	$(CC) $(CFLAGS) -c glsa.c
glsb.o: glsb.c gls.h
	$(RM) $@
	$(CC) $(CFLAGS) -c glsb.c
glsc.o: glsc.c gls.h
	$(RM) $@
	$(CC) $(CFLAGS) -c glsc.c
glsd.o: glsd.c gls.h
	$(RM) $@
	$(CC) $(CFLAGS) -c glsd.c
fitl.o: fitl.c gls.h
	$(RM) $@
	$(CC) $(CFLAGS) -c fitl.c
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
