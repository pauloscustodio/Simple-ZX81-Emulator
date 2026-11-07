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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#undef main   // keep our own main(); we don't need SDLmain.lib's WinMain shim

// global variables
Z80 z80;
BYTE memory[MEMSIZE] = { 0 };
bool do_exit = false;

// static variables
#define MEM_NAME "memory.img"
static char k16_NAME[8] = "000.16k";
static FILE* fptr;
static bool trace = false;

// the screen surface
static SDL_Surface* screen;
// the surface that holds the zx81 charset
static SDL_Surface* charset;

// fetches an opcode from memory
BYTE z80_fetch( Z80* z80, WORD a ) {
    // opcodes fetched below 0x8000 are just read
    if ( a < 0x8000 ) {
        return memory[ a ];
    }

    // opcodes fetched above 0x7fff are read modulo 0x8000 and as 0 if the 6th bit
    // is reset
    BYTE b = memory[ a & 0x7fff ];
    return ( b & 64 ) ? b : 0;
}

// reads from memory
BYTE z80_read( Z80* z80, WORD a ) {
    return memory[ a ];
}

// writes to memory
void z80_write( Z80* z80, WORD a, BYTE b ) {
    // don't write to rom
    if ( a >= RAM_START ) {
        memory[ a ] = b;
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

// creates a sdl surface with the zx81 character set
static int create_charset( void ) {
    SDL_Surface* charset_rgb;
    Uint32 rmask, gmask, bmask, black, white;
    int i, addr, row, col, b;
    Uint32* pixel;

    // create a rgb surface to hold 256 8x8 characters
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
#endif
    charset_rgb = SDL_CreateRGBSurface( SDL_SWSURFACE, 4096, 16, 32, rmask, gmask,
                                        bmask, 0 );

    if ( charset_rgb == NULL ) {
        return 0;
    }

    // map black and white colors
    black = SDL_MapRGB( charset_rgb->format, 0, 0, 0 );
    white = SDL_MapRGB( charset_rgb->format, 255, 255, 255 );

    // pixel points to the top-left pixel of the surface
    pixel = (Uint32*)charset_rgb->pixels;
    // addr points to the start of the characters bits
    addr = 0x1e00;
    // the ammount of uint32s to add to go up/down on line
    int pitch = charset_rgb->pitch / 4;

    // create the 128 characters (64 normal + 64 inverted)
    for ( i = 0; i < 64; i++ ) {
        for ( row = 0; row < 8; row++ ) {
            b = rom[ addr++ ];

            for ( col = 0; col < 8; col++ ) {
                if ( b & 128 ) {
                    pixel[            0 ] = black;
                    pixel[            1 ] = black;
                    pixel[ pitch +    0 ] = black;
                    pixel[ pitch +    1 ] = black;

                    pixel[         2048 ] = white;
                    pixel[         2049 ] = white;
                    pixel[ pitch + 2048 ] = white;
                    pixel[ pitch + 2049 ] = white;
                }
                else {
                    pixel[            0 ] = white;
                    pixel[            1 ] = white;
                    pixel[ pitch +    0 ] = white;
                    pixel[ pitch +    1 ] = white;

                    pixel[         2048 ] = black;
                    pixel[         2049 ] = black;
                    pixel[ pitch + 2048 ] = black;
                    pixel[ pitch + 2049 ] = black;
                }
                // advance pixel to the right
                pixel += 2;
                b <<= 1;
            }
            // advance pixel to the start of the next line
            pixel += pitch * 2 - 16;
        }
        // advance pixel to the top-left of the next character
        pixel -= pitch * 16 - 16;
    }

    // convert the rgb surface to a surface the same format of the screen
    charset = SDL_DisplayFormat( charset_rgb );
    SDL_FreeSurface( charset_rgb );

    return 1;
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

    // add the breakpoint to SLOW_DISP for keyboard queue handling
    //hook_handlers[DISPLAY_6] = SLOW_DISP_hook;
    //hook_handlers[SLOW_DISP] = SLOW_DISP_hook;

    // zero unnused 0x407b - used for p file name
    POKE(FREE2, 0);

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

void save_mem() {
    fptr = fopen(MEM_NAME, "wb"); // w for write, b for binary
    if (fptr != NULL) {
        fwrite(memory, MEMSIZE, 1, fptr);
        fclose(fptr);
    }
}

void load_mem() {
    fptr = fopen(MEM_NAME, "rb"); // r for read, b for binary
    if (fptr != NULL) {
        size_t count = fread(memory, 1, MEMSIZE, fptr);
        if (count != MEMSIZE) {
            die("Error reading memory file: %s\n", MEM_NAME);
        }
        fclose(fptr);
    }
}

void save_16k() {
    sprintf(k16_NAME, "%d.16k",
            memory[0x407b]); // 0x407b=16507 make 16k file name 0-255.16k
    fptr = fopen(k16_NAME, "wb"); // w for write, b for binary
    if (fptr != NULL) {
        fwrite(memory + SAVE_ADDR, 0x4000, 1, fptr);
        fclose(fptr);
    }
}

void load_16k() {
    sprintf(k16_NAME, "%d.16k",
            memory[0x407b]); // 0x407b=16507 make 16k file name 0-255.16k
    fptr = fopen(k16_NAME, "rb"); // r for read, b for binary
    if (fptr != NULL) {
        size_t count = fread(memory + SAVE_ADDR, 1, 0x4000, fptr);
        if (count != 0x4000) {
            die("Error reading 16k file: %s\n", k16_NAME);
        }
        fclose(fptr);
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
            if ( event.key.keysym.sym == SDLK_ESCAPE ) {
                do_exit = true;
            }
            if ( event.key.keysym.sym == SDLK_F8 ) {
                save_16k();
            }
            if ( event.key.keysym.sym == SDLK_F9 ) {
                load_16k();
            }
            if ( event.key.keysym.sym == SDLK_F6 ) {
                save_mem();
            }
            if ( event.key.keysym.sym == SDLK_F7 ) {
                load_mem();
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

static void update_screen( void ) {
    // a pointer to the display file
    WORD d_file;
    // rects to blit from the charset to the screen
    SDL_Rect source, dest;
    // counters to redraw the screen
    int row, col;

    // setup invariants of the rect to address characters in the charset
    source.y = 0;
    source.w = 16;
    source.h = 16;

    // get the pointer into the display file
    d_file = memory[ D_FILE ] | memory[ D_FILE + 1 ] << 8;

    // redraw the screen; we could maintain a copy of the display file to avoid
    // unnecessary blits
    dest.y = 0;

    for ( row = 0; row < 24; row++ ) {
        dest.x = 0;

        for ( col = 0; col < 32; col++ ) {
            source.x = memory[ ++d_file ] * 16;
            SDL_BlitSurface( charset, &source, screen, &dest );
            dest.x += 16;
        }

        // skip the 0x76 at the end of the line
        d_file++;
        dest.y += 16;
    }

    SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

static void exit_usage(void) {
    fprintf(stderr, "Usage: zx81 [-t] file.p\n");
    exit(EXIT_FAILURE);
}


int main( int argc, char* argv[] ) {
    const char* p_file = NULL;

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
        p_file = argv[optind];
        if (!str_ends_with(p_file, ".p")) {
            exit_usage();
        }
    }
    else if (argc != optind) {
        exit_usage();
    }

    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        die("SDL_Init failed: %s\n", SDL_GetError());
    }

    // create our 512x384 screen; the bpp will be the same as the desktop
    screen = SDL_SetVideoMode( 512, 384, 0, SDL_SWSURFACE );
    if ( screen == NULL ) {
        die("Unable to set 512x384 video: %s\n", SDL_GetError());
    }

    // create the characters
    if ( !create_charset() ) {
        SDL_FreeSurface( screen );
        die("Unable to create charset image: %s\n", SDL_GetError());
    }

    // initialize the state
    setup_emulation();
    if (p_file != NULL) {
        push_load_file(p_file);
    }

    // emulate!
    while(!do_exit) {
        run_some();
        consume_events();
        update_screen();
    }

    SDL_FreeSurface( screen );

    return 0;
}
