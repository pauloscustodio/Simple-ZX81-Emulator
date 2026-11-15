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
static kbd_state_t sdlk2kbd_base(SDL_Keycode sym) {
    switch (sym) {
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return K_SHIFT;
    case SDLK_z:
        return K_Z;
    case SDLK_x:
        return K_X;
    case SDLK_c:
        return K_C;
    case SDLK_v:
        return K_V;
    case SDLK_a:
        return K_A;
    case SDLK_s:
        return K_S;
    case SDLK_d:
        return K_D;
    case SDLK_f:
        return K_F;
    case SDLK_g:
        return K_G;
    case SDLK_q:
        return K_Q;
    case SDLK_w:
        return K_W;
    case SDLK_e:
        return K_E;
    case SDLK_r:
        return K_R;
    case SDLK_t:
        return K_T;
    case SDLK_1:
        return K_1;
    case SDLK_2:
        return K_2;
    case SDLK_3:
        return K_3;
    case SDLK_4:
        return K_4;
    case SDLK_5:
        return K_5;
    case SDLK_0:
        return K_0;
    case SDLK_9:
        return K_9;
    case SDLK_8:
        return K_8;
    case SDLK_7:
        return K_7;
    case SDLK_6:
        return K_6;
    case SDLK_p:
        return K_P;
    case SDLK_o:
        return K_O;
    case SDLK_i:
        return K_I;
    case SDLK_u:
        return K_U;
    case SDLK_y:
        return K_Y;
    case SDLK_RETURN:
        return K_RETURN;
    case SDLK_l:
        return K_L;
    case SDLK_k:
        return K_K;
    case SDLK_j:
        return K_J;
    case SDLK_h:
        return K_H;
    case SDLK_SPACE:
        return K_SPACE;
    case SDLK_PERIOD:
        return K_PERIOD;
    case SDLK_m:
        return K_M;
    case SDLK_n:
        return K_N;
    case SDLK_b:
        return K_B;
    case SDLK_BACKSPACE:
        return K_BACKSPACE;
    case SDLK_MINUS:
        return K_MINUS;
    case SDLK_EQUALS:
        return K_EQ;
    case SDLK_LEFTBRACKET:
        return K_LPAREN;
    case SDLK_RIGHTBRACKET:
        return K_RPAREN;
    case SDLK_SEMICOLON:
        return K_SEMICOLON;
    case SDLK_QUOTE:
        return K_QUOTE;
    case SDLK_COMMA:
        return K_COMMA;
    case SDLK_SLASH:
        return K_DIVIDE;
    case SDLK_UP:
        return K_UP;
    case SDLK_DOWN:
        return K_DOWN;
    case SDLK_LEFT:
        return K_LEFT;
    case SDLK_RIGHT:
        return K_RIGHT;
    default:
        return 0;
    }
}

static kbd_state_t sdlk2kbd_shift(SDL_Keycode sym) {
    switch (sym) {
    case SDLK_EQUALS:
        return K_PLUS;
    case SDLK_SEMICOLON:
        return K_COLON;
    case SDLK_COMMA:
        return K_LT;
    case SDLK_PERIOD:
        return K_GT;
    case SDLK_SLASH:
        return K_QUESTION;
    default:
        return sdlk2kbd_base(sym);
    }
}

static kbd_state_t sdlk2kbd(SDL_Keycode sym, bool shifted) {
    return shifted ? sdlk2kbd_shift(sym) : sdlk2kbd_base(sym);
}

//-----------------------------------------------------------------------------
void kbd_init(void) {
    clear_matrix();
    queue_init(&kbd_queue);
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
    SDL_Keycode sym = event->key.keysym.sym;
    bool shifted = (event->key.keysym.mod & KMOD_SHIFT) != 0;

    switch (event->type) {
    case SDL_KEYDOWN: {
        kbd_state_t key = sdlk2kbd(sym, shifted);
        if (key == 0) {
            break;
        }

#ifdef KEYBOARD_ROLLOVER
        // with key rollover but slugish for games
        queue_push(&kbd_queue, key);
#else
        // without rollover but more responsive for games
        keydown(key);
#endif
        break;
    }

    case SDL_KEYUP: {
        kbd_state_t key = sdlk2kbd(sym, shifted);
        if (key == 0) {
            break;
        }

#ifdef KEYBOARD_ROLLOVER
        // do nothing, keyup is handled in the queue_tick() function
#else
        keyup(key);
#endif
        break;
    }
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
