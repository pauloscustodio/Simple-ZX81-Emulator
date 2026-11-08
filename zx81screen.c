//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "simz80.h"
#include "utils.h"
#include "zx81.h"
#include "zx81screen.h"
#include <SDL.h>
#include <stdlib.h>

// the screen surface
static SDL_Surface* screen = NULL;

// the surface that holds the zx81 charset
static SDL_Surface* charset = NULL;

// last charset address
static WORD last_charset_addr = 0;

void zx81screen_free(void) {
    if (charset) {
        SDL_FreeSurface(charset);
    }
    charset = NULL;

    if (screen) {
        SDL_FreeSurface(screen);
    }
    screen = NULL;
}

void zx81screen_init(void) {
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        die("SDL_Init failed: %s\n", SDL_GetError());
    }

    // create our 512x384 screen; the bpp will be the same as the desktop
    screen = SDL_SetVideoMode(512, 384, 0, SDL_SWSURFACE);
    if (screen == NULL) {
        die("Unable to set 512x384 video: %s\n", SDL_GetError());
    }

    atexit(zx81screen_free);
}

// creates a sdl surface with the zx81 character set
static void zx81screen_update_charset(WORD charset_addr) {
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
    charset_rgb = SDL_CreateRGBSurface(SDL_SWSURFACE, 4096, 16, 32, rmask, gmask,
                                       bmask, 0);
    if (charset_rgb == NULL) {
        die("Unable to create RGB surface: %s\n", SDL_GetError());
    }

    // map black and white colors
    black = SDL_MapRGB(charset_rgb->format, 0, 0, 0);
    white = SDL_MapRGB(charset_rgb->format, 255, 255, 255);

    // pixel points to the top-left pixel of the surface
    pixel = (Uint32*)charset_rgb->pixels;

    // addr points to the start of the characters bits
    addr = charset_addr;

    // the ammount of uint32s to add to go up/down on line
    int pitch = charset_rgb->pitch / 4;

    // create the 128 characters (64 normal + 64 inverted)
    for (i = 0; i < 64; i++) {
        for (row = 0; row < 8; row++) {
            b = PEEK(addr++);

            for (col = 0; col < 8; col++) {
                if (b & 128) {
                    pixel[0] = black;
                    pixel[1] = black;
                    pixel[pitch + 0] = black;
                    pixel[pitch + 1] = black;

                    pixel[2048] = white;
                    pixel[2049] = white;
                    pixel[pitch + 2048] = white;
                    pixel[pitch + 2049] = white;
                }
                else {
                    pixel[0] = white;
                    pixel[1] = white;
                    pixel[pitch + 0] = white;
                    pixel[pitch + 1] = white;

                    pixel[2048] = black;
                    pixel[2049] = black;
                    pixel[pitch + 2048] = black;
                    pixel[pitch + 2049] = black;
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
    charset = SDL_DisplayFormat(charset_rgb);
    if (charset == NULL) {
        SDL_FreeSurface(charset_rgb);
        die("Unable to convert charset surface: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(charset_rgb);
}

static void zx81screen_update_display(WORD display_addr) {
    // a pointer to the display file
    WORD addr = display_addr;

    // rects to blit from the charset to the screen
    SDL_Rect source = { 0 }, dest = { 0 };

    // counters to redraw the screen
    int row = 0, col = 0;

    // setup invariants of the rect to address characters in the charset
    source.y = 0;
    source.w = 16;
    source.h = 16;

    // redraw the screen; we could maintain a copy of the display file to avoid
    // unnecessary blits
    dest.y = 0;

    addr++; // skip the 0x76 at the start of the display
    for (row = 0; row < NUM_ROWS; row++) {
        dest.x = 0;

        for (col = 0; col < NUM_COLS; col++) {
            BYTE b = PEEK(addr);
            if (b == 0x76) {
                b = 0; // the 0x76 is a space
            }
            else {
                addr++; // advance to the next byte in the display file
            }

            source.x = b * 16;
            SDL_BlitSurface(charset, &source, screen, &dest);
            dest.x += 16;
        }

        addr++;        // skip the 0x76 at the end of the line
        dest.y += 16;
    }

    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void zx81screen_update(WORD charset_addr, WORD display_addr) {
    if (last_charset_addr != charset_addr) {
        zx81screen_update_charset(charset_addr);
        last_charset_addr = charset_addr;
    }

    if (display_addr != 0) {
        zx81screen_update_display(display_addr);
    }
}
