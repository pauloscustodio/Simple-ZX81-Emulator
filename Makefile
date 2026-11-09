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

CFLAGS 	:= -O3 -MMD -Wall -Werror \
			$(filter-out -Dmain=SDL_main,$(shell pkg-config --cflags sdl))
LDFLAGS := 	$(filter-out -lSDLmain,$(shell pkg-config --libs sdl))

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
FILE2C_OBJS = file2c.o
ZX81_OBJS = $(filter-out $(FILE2C_OBJS), $(OBJS))

FILE2C := $(CURDIR)/file2c$(EXESUFFIX)

DEPS = $(ZX81_OBJS:.o=.d) $(FILE2C_OBJS:.o=.d)

.c.o:
	gcc $(CFLAGS) -c $<

all: zx81$(EXESUFFIX) file2c$(EXESUFFIX)

zx81$(EXESUFFIX): $(ZX81_OBJS)
	gcc $(CFLAGS) -o zx81$(EXESUFFIX) $(ZX81_OBJS) $(LDFLAGS)

zx81.o: zx81rom.h

zx81rom.h: zx81.rom file2c$(EXESUFFIX) Makefile
	"$(FILE2C)" $< rom > $@
	dos2unix zx81rom.h

file2c$(EXESUFFIX): $(FILE2C_OBJS)
	gcc $(CFLAGS) -o file2c$(EXESUFFIX) $(FILE2C_OBJS)

clean::
	$(RM) $(wildcard zx81$(EXESUFFIX) $(ZX81_OBJS) file2c$(EXESUFFIX) $(FILE2C_OBJS) $(DEPS) *.o *.d *.orig *.bak zx81.txt zx81.dump)

-include $(DEPS)

# tidy
STYLE_WIDTH = 80
ASTYLE	= astyle --style=attach --pad-oper --align-pointer=type \
		  --break-closing-braces --add-braces --attach-return-type \
		  --max-code-length=$(STYLE_WIDTH) --lineend=linux --formatted 
PERLTIDY = perltidy --indent-columns=4 --continuation-indentation=4 \
		  --maximum-line-length=$(STYLE_WIDTH) -b
TIDY_STAMP = .tidy-stamp

tidy:
	@[ -f $(TIDY_STAMP) ] || touch -t 197001010000 $(TIDY_STAMP)
	find . \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.re' \) \
		-newer $(TIDY_STAMP) -exec $(ASTYLE) {} +
	find . \( -name '*.pl' -o -name '*.pm' -o -name '*.t' \) \
		-newer $(TIDY_STAMP) -exec $(PERLTIDY) {} +
	touch $(TIDY_STAMP)

# test files
all: hello.p
hello.p: hello.bas
	zx81_make_p -o hello.p hello.bas
clean::
	$(RM) $(wildcard *.p t/input/*.p)

# test
test: zx81$(EXESUFFIX) hello.p bold_font.asm
	perl -S prove t/*.t

all: bold_font.asm
bold_font.asm: zx81.rom make_bold_font.pl Makefile
	perl make_bold_font.pl $< > $@
	dos2unix $@
