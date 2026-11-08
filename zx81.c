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
#include "zx81rom.h"
#include "zx81screen.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#undef main   // keep our own main(); we don't need SDLmain.lib's WinMain shim

// global variables
Z80 z80;
BYTE memory[MEMSIZE] = { 0 };
bool do_exit = false;

// static variables
static bool trace = false;

// fetches an opcode from memory
BYTE z80_fetch( Z80* z80, WORD a ) {
    // opcodes fetched below 0x8000 are just read
    if ( a < 0x8000 ) {
        return PEEK( a );
    }

    // opcodes fetched above 0x7fff are read modulo 0x8000 and as 0 if the 6th bit
    // is reset
    BYTE b = PEEK( a & 0x7fff );
    return ( b & 64 ) ? b : 0;
}

// reads from memory
BYTE z80_read( Z80* z80, WORD a ) {
    return PEEK( a );
}

// writes to memory
void z80_write( Z80* z80, WORD a, BYTE b ) {
    // don't write to rom
    if ( a >= RAM_START ) {
        POKE( a, b );
    }
}

// reads from a port
BYTE z80_in( Z80* z80, WORD addr ) {
    // any read where the 0th bit of the port is zero reads from the keyboard
    if ((addr & 1) == 0) {
        return kbd_input_isr(z80, addr);
    }

    return 0xff;
}

// writes to a port
void z80_out( Z80* z80, WORD a, BYTE b ) {
    (void)z80;
    (void)a;
    (void)b;
}

// setup the emulation state
static void setup_emulation( void ) {
    kbd_init();

    // init memory and z80 state
    memset( &z80, 0, sizeof( z80 ) );
    memset(memory, 0, sizeof(memory));

    // load rom
    memcpy(&memory[0x0000], rom, 0x2000);

    // patch DISPLAY-5 to a return so that it does not execute the display file
    POKE(DISPLAY_5, 0xc9);

    // patch SCROLL to a return, since it doesn't work correctly here
    POKE(SCROLL, 0xc9);

    // setup the registers
    z80.pc  = 0;
    z80.iff = 0;
    z80.af_sel = z80.regs_sel = 0;
}

static void run_some( void ) {
    int count;

    // execute 100000 z80 instructions; the less instructions we execute here the
    // slower the emulation gets, the more we execute the less responsive the
    // keyboard gets
    for ( count = 0; count < 100000; count++ ) {
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
}

static void consume_events( void ) {
    // the event to process the window manager events
    SDL_Event event;

    // empty the event queue
    while ( SDL_PollEvent( &event ) ) {
        switch ( event.type ) {
        case SDL_KEYDOWN:
            kbd_handle_sdl_event(&event);

            // key pressed, reset the corresponding bit in the keyboard state
            if ( event.key.keysym.sym == SDLK_F6 ) {
                write_state(DUMP_NAME, &z80, memory);
            }
            else if ( event.key.keysym.sym == SDLK_F7 ) {
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
    fprintf(stderr, "Usage: zx81 [-t] [file.p|file.dump]\n");
    exit(EXIT_FAILURE);
}

int main( int argc, char* argv[] ) {
    const char* filename = NULL;

    // check command line arguments for a .p file to load
    while (true) {
        int c = getopt(argc, argv, "t");
        if (c == -1) {
            break;
        }
        switch (c) {
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
        run_some();
        consume_events();

        WORD d_file = PEEKW(D_FILE);
        WORD charset_addr = z80.ir & 0xFF00;
        zx81screen_update(charset_addr, d_file);
    }

    zx81screen_free();
    return 0;
}
