#------------------------------------------------------------------------------
# Simple-ZX81-Emulator
# Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
# Modifications Copyright (C) Paulo Custodio, 2026
# License: GPL
#------------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
  EXESUFFIX	:= .exe
else
  EXESUFFIX	?=
endif

CFLAGS 	:= -O3 -MMD -Wall -Werror
LDFLAGS	:= `pkg-config --cflags --libs sdl | sed -e 's/-Dmain=SDL_main//' -e 's/-lSDLmain//'`

ZX81_OBJS = zx81.o simz80.o
FILE2C_OBJS = file2c.o
DEPS = $(ZX81_OBJS:.o=.d) $(FILE2C_OBJS:.o=.d)

.c.o:
	gcc $(CFLAGS) -c $< $(LDFLAGS)

all: zx81$(EXESUFFIX) file2c$(EXESUFFIX)

zx81$(EXESUFFIX): $(ZX81_OBJS)
	gcc $(CFLAGS) -o zx81$(EXESUFFIX) $(ZX81_OBJS) $(LDFLAGS)

zx81.o: zx81rom.h

zx81rom.h: zx81.rom file2c$(EXESUFFIX)
	./file2c$(EXESUFFIX) zx81.rom rom > zx81rom.h
	dos2unix zx81rom.h

file2c$(EXESUFFIX): $(FILE2C_OBJS)
	gcc $(CFLAGS) -o file2c$(EXESUFFIX) $(FILE2C_OBJS)

clean:
	rm -f zx81$(EXESUFFIX) $(ZX81_OBJS) file2c$(EXESUFFIX) $(FILE2C_OBJS) $(DEPS)

-include $(DEPS)
