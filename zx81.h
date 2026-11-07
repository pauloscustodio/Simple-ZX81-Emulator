//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#pragma once

#include "simz80.h"
#include <stdbool.h>

// Memory Size
#define RAM_START 0x2000
#define MEMSIZE   0x10000

// Emulator HOOKS
#define EXIT_HOOK 0x0001    // exit the emulator

// ROM routines
#define RET_ADDR  0x002f    // any ret instruction in ROM
#define DISPLAY_5 0x02b5
#define NEXT_PROG 0x0347
#define DISPLAY_6 0x04c1
#define SLOW_DISP 0x04cf
#define SCROLL    0x0c0e

// system variables
#define SAVE_ADDR 0x4009

#define ERR_NO  0x4000
#define FLAGS   0x4001
#define ERR_SP  0x4002
#define RAMTOP  0x4004
#define MODE    0x4006
#define PPC     0x4007
#define VERSN   0x4009
#define E_PPC   0x400a
#define D_FILE  0x400c
#define DF_CC   0x400e
#define VARS    0x4010
#define DEST    0x4012
#define E_LINE  0x4014
#define CH_ADD  0x4016
#define X_PTR   0x4018
#define STKBOT  0x401a
#define STKEND  0x401c
#define BREG    0x401e
#define MEM     0x401f
#define FREE1   0x4021
#define DF_SZ   0x4022
#define S_TOP   0x4023
#define LAST_K  0x4025
#define DBOUNCE 0x4027
#define MARGIN  0x4028
#define NXTLIN  0x4029
#define OLDPPC  0x402b
#define FLAGX   0x402d
#define STRLEN  0x402e
#define T_ADDR  0x4030
#define SEED    0x4032
#define FRAMES  0x4034
#define COORDS  0x4036
#define COORDX  0x4036
#define COORDY  0x4037
#define PR_CC   0x4038
#define S_POSN  0x4039
#define S_POSNC 0x4039
#define S_POSNL 0x403a
#define CDFLAG  0x403b
#define PRBUFF  0x403c
#define MEMBOT  0x405d
#define FREE2   0x407b

// global variables
extern Z80 z80;          		// the z80 state
extern BYTE memory[];           // the memory
extern bool do_exit;            // true to exit the emulator

// memory access macros
#define PEEK(x) memory[x]
#define PEEKW(x) (memory[x] | (memory[(x)+1]<<8))
#define PEEKW_BE(x) ((memory[x]<<8) | memory[(x)+1])
#define POKE(x,v) memory[x] = (v)
#define POKEW(x,v) do { memory[x] = (v)&0xff; memory[(x)+1] = ((v)>>8)&0xff; } while (0)
#define POKEW_BE(x,v) do { memory[x] = ((v)>>8)&0xff; memory[(x)+1] = (v)&0xff; } while (0)
