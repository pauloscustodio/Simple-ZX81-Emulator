//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "zx81.h"
#include "zx81kbd.h"
#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//-----------------------------------------------------------------------------
// keyboard matrix state
//-----------------------------------------------------------------------------
static kbd_state_t matrix = 0;
static kbd_state_t shift = 0;

static void clear_matrix(void) {
    matrix = 0;
    shift = 0;
}

static void keydown(kbd_state_t kbd) {
    matrix |= kbd;
    if ((kbd & K_SHIFT) != 0) {
        shift = K_SHIFT;
    }
}

static void keyup(kbd_state_t kbd) {
    matrix &= ~kbd;
    if ((kbd & K_SHIFT) != 0) {
        shift = 0;
    }
}

//-----------------------------------------------------------------------------
// keyboard queue
//-----------------------------------------------------------------------------
typedef struct kbd_queue_t {
    kbd_state_t data[KBD_QUEUE_SIZE];
    size_t  head;
    size_t  tail;
    size_t  count;
} kbd_queue_t;

static kbd_queue_t kbd_queue = {0};

static uint32_t kbd_read_count = 0;
static bool kbd_state = false;      // true for keydown, false for keyup
static kbd_state_t key_pressed =
    0; // last key injected into the keyboard matrix

static void queue_init(kbd_queue_t* q) {
    memset(&q->data[0], 0, sizeof(q->data));
    q->head = q->tail = q->count = 0;
}

static bool queue_is_empty(kbd_queue_t* q) {
    return q->count == 0;
}

static bool queue_is_full(kbd_queue_t* q) {
    return q->count == KBD_QUEUE_SIZE;
}

static void queue_push(kbd_queue_t* q, kbd_state_t kbd) {
    if (queue_is_full(q)) {
        return;    // NOTE: key is lost
    }
    q->data[q->tail] = kbd;
    q->tail = (q->tail + 1) % KBD_QUEUE_SIZE;
    q->count++;
}

static kbd_state_t queue_pop(kbd_queue_t* q) {
    if (queue_is_empty(q)) {
        return 0;    // NOTE: returns no key pressed
    }
    kbd_state_t kbd = q->data[q->head];
    q->head = (q->head + 1) % KBD_QUEUE_SIZE;
    q->count--;
    return kbd;
}

// call regularly to update the keyboard queue and matrix state
static void queue_tick(void) {
    kbd_read_count++;
    if (kbd_read_count >= KBD_READ_COUNT) {
        kbd_read_count = 0;
        kbd_state = !kbd_state;

        if (kbd_state == true && !queue_is_empty(&kbd_queue)) {
            // pop a key from the keyboard queue
            key_pressed = queue_pop(&kbd_queue);
            keydown(key_pressed);
        }
        else if (kbd_state == false && key_pressed) {
            keyup(key_pressed);
            key_pressed = 0;
        }
    }
}

//-----------------------------------------------------------------------------
// mapping of SDLK_xxx to kbd_state_t
//-----------------------------------------------------------------------------
static kbd_state_t sdlk2kbd[SDLK_LAST];
static kbd_state_t sdlk2kbd_shift[SDLK_LAST];

static void sdlk2kbd_init(void) {
    memset(sdlk2kbd, 0, sizeof(sdlk2kbd));

    sdlk2kbd[SDLK_LSHIFT]       = K_SHIFT;
    sdlk2kbd[SDLK_RSHIFT]       = K_SHIFT;
    sdlk2kbd[SDLK_z]            = K_Z;
    sdlk2kbd[SDLK_x]            = K_X;
    sdlk2kbd[SDLK_c]            = K_C;
    sdlk2kbd[SDLK_v]            = K_V;
    sdlk2kbd[SDLK_a]            = K_A;
    sdlk2kbd[SDLK_s]            = K_S;
    sdlk2kbd[SDLK_d]            = K_D;
    sdlk2kbd[SDLK_f]            = K_F;
    sdlk2kbd[SDLK_g]            = K_G;
    sdlk2kbd[SDLK_q]            = K_Q;
    sdlk2kbd[SDLK_w]            = K_W;
    sdlk2kbd[SDLK_e]            = K_E;
    sdlk2kbd[SDLK_r]            = K_R;
    sdlk2kbd[SDLK_t]            = K_T;
    sdlk2kbd[SDLK_1]            = K_1;
    sdlk2kbd[SDLK_2]            = K_2;
    sdlk2kbd[SDLK_3]            = K_3;
    sdlk2kbd[SDLK_4]            = K_4;
    sdlk2kbd[SDLK_5]            = K_5;
    sdlk2kbd[SDLK_0]            = K_0;
    sdlk2kbd[SDLK_9]            = K_9;
    sdlk2kbd[SDLK_8]            = K_8;
    sdlk2kbd[SDLK_7]            = K_7;
    sdlk2kbd[SDLK_6]            = K_6;
    sdlk2kbd[SDLK_p]            = K_P;
    sdlk2kbd[SDLK_o]            = K_O;
    sdlk2kbd[SDLK_i]            = K_I;
    sdlk2kbd[SDLK_u]            = K_U;
    sdlk2kbd[SDLK_y]            = K_Y;
    sdlk2kbd[SDLK_RETURN]       = K_RETURN;
    sdlk2kbd[SDLK_l]            = K_L;
    sdlk2kbd[SDLK_k]            = K_K;
    sdlk2kbd[SDLK_j]            = K_J;
    sdlk2kbd[SDLK_h]            = K_H;
    sdlk2kbd[SDLK_SPACE]        = K_SPACE;
    sdlk2kbd[SDLK_PERIOD]       = K_PERIOD;
    sdlk2kbd[SDLK_m]            = K_M;
    sdlk2kbd[SDLK_n]            = K_N;
    sdlk2kbd[SDLK_b]            = K_B;
    sdlk2kbd[SDLK_BACKSPACE]    = K_BACKSPACE;
    sdlk2kbd[SDLK_MINUS]        = K_MINUS;
    sdlk2kbd[SDLK_EQUALS]       = K_EQ;
    sdlk2kbd[SDLK_LEFTBRACKET]  = K_LPAREN;
    sdlk2kbd[SDLK_RIGHTBRACKET] = K_RPAREN;
    sdlk2kbd[SDLK_SEMICOLON]    = K_SEMICOLON;
    sdlk2kbd[SDLK_QUOTE]        = K_QUOTE;
    sdlk2kbd[SDLK_COMMA]        = K_COMMA;
    sdlk2kbd[SDLK_SLASH]        = K_DIVIDE;
    sdlk2kbd[SDLK_UP]           = K_UP;
    sdlk2kbd[SDLK_DOWN]         = K_DOWN;
    sdlk2kbd[SDLK_LEFT]         = K_LEFT;
    sdlk2kbd[SDLK_RIGHT]        = K_RIGHT;

    memcpy(sdlk2kbd_shift, sdlk2kbd, sizeof(sdlk2kbd));
    sdlk2kbd_shift[SDLK_EQUALS]    = K_PLUS;
    sdlk2kbd_shift[SDLK_SEMICOLON] = K_COLON;
    sdlk2kbd_shift[SDLK_COMMA]     = K_LT;
    sdlk2kbd_shift[SDLK_PERIOD]    = K_GT;
    sdlk2kbd_shift[SDLK_SLASH]     = K_QUESTION;
}

//-----------------------------------------------------------------------------
void kbd_init(void) {
    clear_matrix();
    queue_init(&kbd_queue);
    sdlk2kbd_init();
}

//-----------------------------------------------------------------------------
static BYTE kbd_in(WORD addr) {
    BYTE row = (BYTE)((addr >> 8) & 0xFF);
    // Note: matrix is 1 for key pressed, ROM is 0 for key pressed
    switch (row) {
    case 0xFE:
        return (BYTE)(~matrix & 0xff);
    case 0xFD:
        return (BYTE)((~matrix >> 8) & 0xff);
    case 0xFB:
        return (BYTE)((~matrix >> 16) & 0xff);
    case 0xF7:
        return (BYTE)((~matrix >> 24) & 0xff);
    case 0xEF:
        return (BYTE)((~matrix >> 32) & 0xff);
    case 0xDF:
        return (BYTE)((~matrix >> 40) & 0xff);
    case 0xBF:
        return (BYTE)((~matrix >> 48) & 0xff);
    case 0x7F:
        return (BYTE)((~matrix >> 56) & 0xff);
    default:
        return 0xff;
    }
}

//-----------------------------------------------------------------------------
BYTE kbd_input_isr(Z80* z80, WORD addr) {
    // first dispatch keys from the keyboard queue
    // run on the first read for a keyboard scan
    // and on 0x7ffe for a break key check
    if (addr == 0xfefe || addr == 0x7ffe) {
        queue_tick();
    }

    // then dispatch keys from the keyboard matrix
    return kbd_in(addr);
}

//-----------------------------------------------------------------------------
void kbd_handle_sdl_event(const SDL_Event* event) {
    SDLKey sym = event->key.keysym.sym;
    bool shifted = (event->key.keysym.mod & KMOD_SHIFT) != 0;

    switch (event->type) {
    case SDL_KEYDOWN:
        if (shifted) {
#ifdef AUTHENTIC_BEHAVIOR
            // key rollover not possible
            keydown(sdlk2kbd_shift[sym]);
#else
            // key rollover possible
            queue_push(&kbd_queue, sdlk2kbd_shift[sym]);
#endif
        }
        else {
#ifdef AUTHENTIC_BEHAVIOR
            // key rollover not possible
            keydown(sdlk2kbd[sym]);
#else
            // key rollover possible
            queue_push(&kbd_queue, sdlk2kbd[sym]);
#endif
        }
        break;

    case SDL_KEYUP:
#ifdef AUTHENTIC_BEHAVIOR
        if (shifted) {
            keyup(sdlk2kbd_shift[event->key.keysym.sym]);
        }
        else {
            keyup(sdlk2kbd[event->key.keysym.sym]);
        }
#endif
        break;
    }
}

//-----------------------------------------------------------------------------
void kbd_push_key(kbd_state_t key) {
    queue_push(&kbd_queue, key);
}

// C translation of the ZX81 KEYBOARD scanning routine
WORD keyboard_scan(void) {
    // one frame has passed, update the keyboard queue and matrix state
    queue_tick();

    // now read the keyboard matrix using the ZX81 keyboard scanning routine
    BYTE h = 0xFF;
    BYTE l = 0xFF;
    BYTE row = 0xFE;

    BYTE a = kbd_in(0xFEFE);
    a |= 0x01;       /* ignore SHIFT initially */

    for (;;) {
        BYTE d = a | 0xE0;

        if ((~a & 0x1F) == 0) {
            l &= 0xFF;
        }
        else {
            l &= row;
        }

        h &= d;

        int done = (row & 0x80) == 0;

        row = (BYTE)((row << 1) | (row >> 7));
        a = kbd_in(((WORD)row << 8) | 0xFE);

        if (done) {
            break;
        }
    }

    /* incorporate SHIFT key */
    if (a & 0x01) {
        h = (BYTE)((h << 1) | 1);
    }
    else {
        h = (BYTE)(h << 1);
    }

    return ((WORD)h << 8) | l;
}
