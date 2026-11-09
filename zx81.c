//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "disasm.h"
#include "getopt.h"
#include "simz80.h"
#include "utils.h"
#include "zx81.h"
#include "zx81kbd.h"
#include "zx81load.h"
#include "zx81printer.h"
#include "zx81rom.h"
#include "zx81screen.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <stdint.h>
#undef main   // keep our own main(); we don't need SDLmain.lib's WinMain shim

// global variables
Z80 z80;
BYTE memory[MEMSIZE] = { 0 };
bool do_exit = false;
bool in_pause = false;
Uint32 pause_start = 0;     // SDL ticks (in ms) when the pause started
Uint32 pause_duration = 0;
bool slow_mode = false;     // true to run at clock speed

// static variables
static bool trace = false;

// fetches an opcode from memory
BYTE z80_fetch(Z80* z80, WORD a) {
    // opcodes fetched below 0x8000 are just read
    if (a < 0x8000) {
        return PEEK(a);
    }

    // opcodes fetched above 0x7fff are read modulo 0x8000 and as 0 if the 6th bit
    // is reset
    BYTE b = PEEK(a & 0x7fff);
    return (b & 64) ? b : 0;
}

// reads from memory
BYTE z80_read(Z80* z80, WORD a) {
    return PEEK(a);
}

// writes to memory
void z80_write(Z80* z80, WORD a, BYTE b) {
    // don't write to rom
    if (a >= RAM_START) {
        POKE(a, b);
    }
}

// reads from a port
BYTE z80_in(Z80* z80, WORD addr) {
    // any read where the 0th bit of the port is zero reads from the keyboard
    if ((addr & 1) == 0) {
        return kbd_input_isr(z80, addr);
    }

    return 0xff;
}

// writes to a port
void z80_out(Z80* z80, WORD a, BYTE b) {
    (void)z80;
    (void)a;
    (void)b;
}

// setup the emulation state
static void setup_emulation(void) {
    kbd_init();

    // init memory and z80 state
    memset(&z80, 0, sizeof(z80));
    memset(memory, 0, sizeof(memory));

    // load rom
    memcpy(&memory[0x0000], rom, 0x2000);

    // patch DISPLAY-5 to a return so that it does not execute the display file
    POKE(DISPLAY_5, 0xc9);

    // patch CDFLAG for SLOW mode
    POKE(CDFLAG, 0x40);

    // setup the registers
    z80.pc  = 0;
    z80.iff = 0;
    z80.af_sel = z80.regs_sel = 0;
}

static void run_frame(void) {
    // execute one video-frame time of z80 instructions
    bool cpu_slow = (PEEK(CDFLAG) & 0x40) != 0;
    bool slow = slow_mode && cpu_slow;
    uint64_t cycles_per_frame = slow ?
                                CYCLES_PER_FRAME_SLOW : CYCLES_PER_FRAME_FAST;

    uint64_t start_ticks = z80.ticks;
    Uint32 start_sdl_ticks = SDL_GetTicks();
    while ((uint64_t)(z80.ticks - start_ticks) < cycles_per_frame) {
        // hooks
        if (z80.pc == EXIT_HOOK) {      // EXIT routine
            do_exit = true;
            return;
        }
        else if (z80.pc == DUMP_HOOK) { // dump memory and z80 state to a file
            write_state(DUMP_NAME, &z80, memory);
        }
        else if (z80.pc == NEXT_PROG) { // LOAD routine after file name was parsed
            next_prog_hook(&z80);
        }
        else if (z80.pc == SAVE1) {     // SAVE routine after file name was parsed
            save_prog_hook(&z80);
        }
        else if (z80.pc == PAUSE1) {    // PAUSE routine after delay parsed
            WORD bc = z80.regs[z80.regs_sel].bc;    // delay
            in_pause = true;
            pause_start = SDL_GetTicks();
            pause_duration = bc * 1000 / REFRESH_HZ;
            z80.pc = RET_ADDR;
        }
        else if (z80.pc == FAST) {      // FAST routine
            POKE(CDFLAG, PEEK(CDFLAG) & ~0x40);
            z80.pc = RET_ADDR;
        }
        else if (z80.pc == SLOW) {      // SLOW routine
            POKE(CDFLAG, PEEK(CDFLAG) | 0x40);
            z80.pc = RET_ADDR;
        }
        else if ((z80.pc == PRINT_A ||
                  z80.pc == PRINT_CH ||
                  z80.pc == PRINT_SP) &&
                 (PEEK(FLAGS) & 2) != 0) {	// print to printer
            lprint_char((z80.af[z80.af_sel]) >> 8);
            z80.pc = RET_ADDR;
        }
        else if (z80.pc == COPY) {
            copy();
            z80.pc = RET_ADDR;
        }

        // handle PAUSE
        if (in_pause) {
            Uint32 elapsed = SDL_GetTicks() - pause_start;
            if (elapsed < pause_duration) {
                // stay in pause, advance
                z80.ticks += 4; // simulate a NOP
                continue;
            }
            else {
                // exit pause, continue execution
                in_pause = false;
            }
        }

        // show the trace if requested
        if (trace) {
            printf("\n");
            disasm(&z80, z80.pc);
        }

        // single-step the z80
        z80_step(&z80);

        // show registers
        if (trace) {
            show_regs(&z80);
        }
    }

    // decrement FRAMES
    WORD frames = PEEKW(FRAMES);
    frames--;
    frames |= 0x8000; // bit 15 must be set
    POKEW(FRAMES, frames);

    // if slow mode is requested, wait until the frame time has passed
    if (slow) {
        Uint32 elapsed = SDL_GetTicks() - start_sdl_ticks;
        if (elapsed < 1000 / REFRESH_HZ) {
            Uint32 delay = 1000 / REFRESH_HZ - elapsed;
            SDL_Delay(delay);
        }
    }
}

static void consume_events(void) {
    // the event to process the window manager events
    SDL_Event event;

    // empty the event queue
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            in_pause = false; // exit pause on any key press
            kbd_handle_sdl_event(&event);

            // key pressed, reset the corresponding bit in the keyboard state
            if (event.key.keysym.sym == SDLK_F6) {
                write_state(DUMP_NAME, &z80, memory);
            }
            else if (event.key.keysym.sym == SDLK_F7) {
                read_state(DUMP_NAME, &z80, memory);
            }
            break;

        case SDL_KEYUP:
            kbd_handle_sdl_event(&event);
            break;

        case SDL_QUIT:
            // quit the emulation
            do_exit = true;
            break;
        }
    }
}

static void exit_usage(void) {
    fprintf(stderr, "Usage: zx81 [-s] [-t] [file.p|file.dump]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    const char* filename = NULL;

    // check command line arguments for a .p file to load
    while (true) {
        int c = getopt(argc, argv, "st");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 's':
            slow_mode = true;
            break;
        case 't':
            trace = true;
            break;
        default:
            exit_usage();
        }
    }

    if (argc == optind + 1) {
        filename = argv[optind];
    }
    else if (argc != optind) {
        exit_usage();
    }

    // initialize the state
    zx81screen_init();
    setup_emulation();

    if (filename != NULL) {
        if (str_ends_with(filename, ".p")) {
            push_load_file(filename);
        }
        else if (str_ends_with(filename, ".dump")) {
            read_state(filename, &z80, memory);
        }
        else {
            die("Invalid file extension: %s\n", filename);
        }
    }

    // emulate
    while(!do_exit) {
        run_frame();
        consume_events();

        WORD d_file = PEEKW(D_FILE);
        WORD charset_addr = z80.ir & 0xFF00;
        zx81screen_update(charset_addr, d_file);
    }

    zx81screen_free();
    return 0;
}
