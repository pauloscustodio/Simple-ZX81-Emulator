#------------------------------------------------------------------------------
# Simple-ZX81-Emulator
# Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
# Modifications Copyright (C) Paulo Custodio, 2026
# License: GPL
#------------------------------------------------------------------------------

PROJ = zx81

ifeq ($(OS),Windows_NT)
  EXESUFFIX	:= .exe
else
  EXESUFFIX	?=
endif

#------------------------------------------------------------------------------
# compile options
#------------------------------------------------------------------------------
CFLAGS 	:= -O3 -MMD -Wall -Werror \
			$(filter-out -Dmain=SDL_main,$(shell pkg-config --cflags sdl))
LDFLAGS := 	$(filter-out -lSDLmain,$(shell pkg-config --libs sdl))

INSTALL ?= install

#------------------------------------------------------------------------------
# astyle
#------------------------------------------------------------------------------
STYLE_WIDTH = 80
ASTYLE	= astyle --style=attach --pad-oper --align-pointer=type \
		  --break-closing-braces --add-braces --attach-return-type \
		  --max-code-length=$(STYLE_WIDTH) --lineend=linux --formatted 
PERLTIDY = perltidy --indent-columns=4 --continuation-indentation=4 \
		  --maximum-line-length=$(STYLE_WIDTH) -b
TIDY_STAMP = .tidy-stamp

#------------------------------------------------------------------------------
# target files
#------------------------------------------------------------------------------
BUILD_DIR = build

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

DEPENDS = $(OBJS:.o=.d)

#------------------------------------------------------------------------------
# build
#------------------------------------------------------------------------------

all: $(PROJ)$(EXESUFFIX)

$(PROJ)$(EXESUFFIX): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@ 

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir $@

$(PROJ).o: zx81rom.h

zx81rom.h: zx81.rom tools/file2c.pl Makefile
	perl tools/file2c.pl $< rom > $@
	dos2unix $@

#------------------------------------------------------------------------------
# test
#------------------------------------------------------------------------------
all: hello.p

hello.p: hello.bas
	zx81_make_p -o hello.p hello.bas

clean::
	$(RM) $(wildcard *.p t/input/*.p)

test: $(PROJ)$(EXESUFFIX) hello.p normal_font.asm bold_font.asm
	perl -S prove t/*.t

all: normal_font.asm bold_font.asm
bold_font.asm: zx81.rom tools/make_bold_font.pl Makefile
	perl tools/make_bold_font.pl $< > $@
	dos2unix $@

normal_font.asm: zx81.rom tools/extract_font.pl Makefile
	perl tools/extract_font.pl $< > $@
	dos2unix $@

inittests: t/input/cpu_test_z80_ok.bas

t/input/cpu_test_z80_ok.bas: t/input/cpu_test_z80_ok.asm \
							tools/make_cpu_test_z80_ok_bas.pl Makefile
	z88dk-z80asm -b t/input/cpu_test_z80_ok.asm
	perl tools/make_cpu_test_z80_ok_bas.pl t/input/cpu_test_z80_ok.bin
	$(RM) t/input/cpu_test_z80_ok.lis \
		  t/input/cpu_test_z80_ok.o \
		  t/input/cpu_test_z80_ok.bin
		  
#------------------------------------------------------------------------------
# tidy
#------------------------------------------------------------------------------
tidy:
	@[ -f $(TIDY_STAMP) ] || touch -t 197001010000 $(TIDY_STAMP)
	find . \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.re' \) \
		-newer $(TIDY_STAMP) -exec $(ASTYLE) {} +
	find . \( -name '*.pl' -o -name '*.pm' -o -name '*.t' \) \
		-newer $(TIDY_STAMP) -exec $(PERLTIDY) {} +
	touch $(TIDY_STAMP)

#------------------------------------------------------------------------------
# clean
#------------------------------------------------------------------------------
clean::
	$(RM) $(wildcard $(PROJ)$(EXESUFFIX) $(OBJS) $(DEPENDS) \
					*.o *.d *.orig *.bak $(PROJ).txt $(PROJ).dump)
	$(RM) -r $(BUILD_DIR)

#------------------------------------------------------------------------------
-include $(DEPENDS)
