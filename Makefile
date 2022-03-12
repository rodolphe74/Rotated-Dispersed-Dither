
# The name of your C compiler:
CC=gcc

# You may need to adjust these cc options:
CFLAGS= -g -I. -std=c99

# Link-time cc options:
LDFLAGS= -lm

# To link any special libraries, add the necessary -l commands here.
LDLIBS= 

# miscellaneous OS-dependent stuff
LN=$(CC)
RM=rm -f
CP=cp
MV=mv
AR=ar rc
AR2=ranlib

# End of configurable options.


all: rotdither rotdithercmy rotditherrgb rotdithertest

rotdither:	rot_dither.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
rotdithercmy:	rot_dither_cmy.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
rotditherrgb:	rot_dither_rgb.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
rotdithertest:	rot_dither_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)


clean:
	$(RM) rotdither.exe
	$(RM) rotdithertest.exe
	$(RM) rotdithercmy.exe
	$(RM) rotditherrgb.exe
