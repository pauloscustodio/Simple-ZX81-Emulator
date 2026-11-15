//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "utils.h"
#include "utstring.h"
#include "zx81.h"
#include "zx81kbd.h"
#include <ctype.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
    kbd_state_t* data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} kbd_queue_t;

static kbd_queue_t kbd_queue = {0};

static Uint32 kbd_last_event_ms = 0;

// true for keydown, false for keyup
static bool kbd_state = false;

// last key injected into the keyboard matrix
static kbd_state_t key_pressed = 0;

static void queue_init(kbd_queue_t* q) {
    if (q->data != NULL) {
        free(q->data);
    }
    q->capacity = KBD_QUEUE_SIZE;
    q->data = (kbd_state_t*)xmalloc(q->capacity * sizeof(kbd_state_t));
    q->head = q->tail = q->count = 0;
}

static bool queue_is_empty(kbd_queue_t* q) {
    return q->count == 0;
}

static void queue_push(kbd_queue_t* q, kbd_state_t kbd) {
    if (q->count == q->capacity) {
        size_t new_capacity = (q->capacity == 0) ? KBD_QUEUE_SIZE : q->capacity * 2;
        kbd_state_t* new_data = (kbd_state_t*)xmalloc(new_capacity * sizeof(
                                    kbd_state_t));

        for (size_t i = 0; i < q->count; i++) {
            new_data[i] = q->data[(q->head + i) % q->capacity];
        }

        free(q->data);
        q->data = new_data;
        q->capacity = new_capacity;
        q->head = 0;
        q->tail = q->count;
    }

    q->data[q->tail] = kbd;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
}

static kbd_state_t queue_pop(kbd_queue_t* q) {
    if (queue_is_empty(q)) {
        return 0;    // NOTE: returns no key pressed
    }
    kbd_state_t kbd = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    if (q->count == 0) {
        q->head = 0;
        q->tail = 0;
    }
    return kbd;
}

// call regularly to update the keyboard queue and matrix state
static void queue_tick(void) {
    Uint32 kbd_current_ms = SDL_GetTicks();
    if (kbd_current_ms - kbd_last_event_ms >= KBD_READ_MS / 2) {
        kbd_last_event_ms = kbd_current_ms;
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
    queue_tick();

    // then dispatch keys from the keyboard matrix
    return kbd_in(addr);
}

//-----------------------------------------------------------------------------
void kbd_handle_sdl_event(const SDL_Event* event) {
    SDL_Keycode sym = event->key.keysym.sym;
    bool shifted = (event->key.keysym.mod & KMOD_SHIFT) != 0;
    bool control = (event->key.keysym.mod & (KMOD_CTRL | KMOD_GUI)) != 0;

    switch (event->type) {
    case SDL_KEYDOWN: {
        // Paste
        if (control && event->key.keysym.sym == SDLK_v) {
            // Paste
            char* text = SDL_GetClipboardText();
            if (text) {
                basic_paste(text);
                SDL_free(text);
            }
            break;
        }
        /*
        if (control && event->key.keysym.sym == SDLK_v)
        {
            printf("paste\n");
            break;
        }
        */

        // other keys
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

//-----------------------------------------------------------------------------
enum { KL_MODE, FN_MODE, GR_MODE };

typedef struct {
    const char* keyword;
    int mode;
    kbd_state_t key;
} keyword_map_t;

static const keyword_map_t keyword_map[256] = {
    /* 0x00 */ { " ", KL_MODE, K_SPACE },
    /* 0x01 */ { "\\' ", GR_MODE, K_SHIFT | K_1 },
    /* 0x02 */ { "\\ '", GR_MODE, K_SHIFT | K_2 },
    /* 0x03 */ { "\\''", GR_MODE, K_SHIFT | K_7 },
    /* 0x04 */ { "\\. ", GR_MODE, K_SHIFT | K_4 },
    /* 0x05 */ { "\\: ", GR_MODE, K_SHIFT | K_5 },
    /* 0x06 */ { "\\.'", GR_MODE, K_SHIFT | K_T },
    /* 0x07 */ { "\\:'", GR_MODE, K_SHIFT | K_E },
    /* 0x08 */ { "\\##", GR_MODE, K_SHIFT | K_A },
    /* 0x09 */ { "\\,,", GR_MODE, K_SHIFT | K_D },
    /* 0x0A */ { "\\~~", GR_MODE, K_SHIFT | K_S },
    /* 0x0B */ { "\"", KL_MODE, K_QUOTE },
    /* 0x0C */ { "#", KL_MODE, K_POUND },
    /* 0x0D */ { "$", KL_MODE, K_DOLLAR },
    /* 0x0E */ { ":", KL_MODE, K_COLON },
    /* 0x0F */ { "?", KL_MODE, K_QUESTION },
    /* 0x10 */ { "(", KL_MODE, K_LPAREN },
    /* 0x11 */ { ")", KL_MODE, K_RPAREN },
    /* 0x12 */ { ">", KL_MODE, K_GT },
    /* 0x13 */ { "<", KL_MODE, K_LT },
    /* 0x14 */ { "=", KL_MODE, K_EQ },
    /* 0x15 */ { "+", KL_MODE, K_PLUS },
    /* 0x16 */ { "-", KL_MODE, K_MINUS },
    /* 0x17 */ { "*", KL_MODE, K_MULTIPLY },
    /* 0x18 */ { "/", KL_MODE, K_DIVIDE },
    /* 0x19 */ { ";", KL_MODE, K_SEMICOLON },
    /* 0x1A */ { ",", KL_MODE, K_COMMA },
    /* 0x1B */ { ".", KL_MODE, K_PERIOD },
    /* 0x1C */ { "0", KL_MODE, K_0 },
    /* 0x1D */ { "1", KL_MODE, K_1 },
    /* 0x1E */ { "2", KL_MODE, K_2 },
    /* 0x1F */ { "3", KL_MODE, K_3 },
    /* 0x20 */ { "4", KL_MODE, K_4 },
    /* 0x21 */ { "5", KL_MODE, K_5 },
    /* 0x22 */ { "6", KL_MODE, K_6 },
    /* 0x23 */ { "7", KL_MODE, K_7 },
    /* 0x24 */ { "8", KL_MODE, K_8 },
    /* 0x25 */ { "9", KL_MODE, K_9 },
    /* 0x26 */ { "A", KL_MODE, K_A },
    /* 0x27 */ { "B", KL_MODE, K_B },
    /* 0x28 */ { "C", KL_MODE, K_C },
    /* 0x29 */ { "D", KL_MODE, K_D },
    /* 0x2A */ { "E", KL_MODE, K_E },
    /* 0x2B */ { "F", KL_MODE, K_F },
    /* 0x2C */ { "G", KL_MODE, K_G },
    /* 0x2D */ { "H", KL_MODE, K_H },
    /* 0x2E */ { "I", KL_MODE, K_I },
    /* 0x2F */ { "J", KL_MODE, K_J },
    /* 0x30 */ { "K", KL_MODE, K_K },
    /* 0x31 */ { "L", KL_MODE, K_L },
    /* 0x32 */ { "M", KL_MODE, K_M },
    /* 0x33 */ { "N", KL_MODE, K_N },
    /* 0x34 */ { "O", KL_MODE, K_O },
    /* 0x35 */ { "P", KL_MODE, K_P },
    /* 0x36 */ { "Q", KL_MODE, K_Q },
    /* 0x37 */ { "R", KL_MODE, K_R },
    /* 0x38 */ { "S", KL_MODE, K_S },
    /* 0x39 */ { "T", KL_MODE, K_T },
    /* 0x3A */ { "U", KL_MODE, K_U },
    /* 0x3B */ { "V", KL_MODE, K_V },
    /* 0x3C */ { "W", KL_MODE, K_W },
    /* 0x3D */ { "X", KL_MODE, K_X },
    /* 0x3E */ { "Y", KL_MODE, K_Y },
    /* 0x3F */ { "Z", KL_MODE, K_Z },
    /* 0x40 */ { "RND", FN_MODE, FN_RND },
    /* 0x41 */ { "INKEY$", FN_MODE, FN_INKEY_DLR },
    /* 0x42 */ { "PI", FN_MODE, FN_PI },
    /* 0x43 */ { NULL, 0, 0 },
    /* 0x44 */ { NULL, 0, 0 },
    /* 0x45 */ { NULL, 0, 0 },
    /* 0x46 */ { NULL, 0, 0 },
    /* 0x47 */ { NULL, 0, 0 },
    /* 0x48 */ { NULL, 0, 0 },
    /* 0x49 */ { NULL, 0, 0 },
    /* 0x4A */ { NULL, 0, 0 },
    /* 0x4B */ { NULL, 0, 0 },
    /* 0x4C */ { NULL, 0, 0 },
    /* 0x4D */ { NULL, 0, 0 },
    /* 0x4E */ { NULL, 0, 0 },
    /* 0x4F */ { NULL, 0, 0 },
    /* 0x50 */ { NULL, 0, 0 },
    /* 0x51 */ { NULL, 0, 0 },
    /* 0x52 */ { NULL, 0, 0 },
    /* 0x53 */ { NULL, 0, 0 },
    /* 0x54 */ { NULL, 0, 0 },
    /* 0x55 */ { NULL, 0, 0 },
    /* 0x56 */ { NULL, 0, 0 },
    /* 0x57 */ { NULL, 0, 0 },
    /* 0x58 */ { NULL, 0, 0 },
    /* 0x59 */ { NULL, 0, 0 },
    /* 0x5A */ { NULL, 0, 0 },
    /* 0x5B */ { NULL, 0, 0 },
    /* 0x5C */ { NULL, 0, 0 },
    /* 0x5D */ { NULL, 0, 0 },
    /* 0x5E */ { NULL, 0, 0 },
    /* 0x5F */ { NULL, 0, 0 },
    /* 0x60 */ { NULL, 0, 0 },
    /* 0x61 */ { NULL, 0, 0 },
    /* 0x62 */ { NULL, 0, 0 },
    /* 0x63 */ { NULL, 0, 0 },
    /* 0x64 */ { NULL, 0, 0 },
    /* 0x65 */ { NULL, 0, 0 },
    /* 0x66 */ { NULL, 0, 0 },
    /* 0x67 */ { NULL, 0, 0 },
    /* 0x68 */ { NULL, 0, 0 },
    /* 0x69 */ { NULL, 0, 0 },
    /* 0x6A */ { NULL, 0, 0 },
    /* 0x6B */ { NULL, 0, 0 },
    /* 0x6C */ { NULL, 0, 0 },
    /* 0x6D */ { NULL, 0, 0 },
    /* 0x6E */ { NULL, 0, 0 },
    /* 0x6F */ { NULL, 0, 0 },
    /* 0x70 */ { NULL, 0, 0 },
    /* 0x71 */ { NULL, 0, 0 },
    /* 0x72 */ { NULL, 0, 0 },
    /* 0x73 */ { NULL, 0, 0 },
    /* 0x74 */ { NULL, 0, 0 },
    /* 0x75 */ { NULL, 0, 0 },
    /* 0x76 */ { NULL, 0, 0 },
    /* 0x77 */ { NULL, 0, 0 },
    /* 0x78 */ { NULL, 0, 0 },
    /* 0x79 */ { NULL, 0, 0 },
    /* 0x7A */ { NULL, 0, 0 },
    /* 0x7B */ { NULL, 0, 0 },
    /* 0x7C */ { NULL, 0, 0 },
    /* 0x7D */ { NULL, 0, 0 },
    /* 0x7E */ { NULL, 0, 0 },
    /* 0x7F */ { NULL, 0, 0 },
    /* 0x80 */ { "\\::", GR_MODE, K_SPACE },
    /* 0x81 */ { "\\.:", GR_MODE, K_SHIFT | K_Q },
    /* 0x82 */ { "\\:.", GR_MODE, K_SHIFT | K_W },
    /* 0x83 */ { "\\..", GR_MODE, K_SHIFT | K_6 },
    /* 0x84 */ { "\\':", GR_MODE, K_SHIFT | K_R },
    /* 0x85 */ { "\\ :", GR_MODE, K_SHIFT | K_8 },
    /* 0x86 */ { "\\'.", GR_MODE, K_SHIFT | K_Y },
    /* 0x87 */ { "\\ .", GR_MODE, K_SHIFT | K_3 },
    /* 0x88 */ { "\\@@", GR_MODE, K_SHIFT | K_H },
    /* 0x89 */ { "\\;;", GR_MODE, K_SHIFT | K_G },
    /* 0x8A */ { "\\!!", GR_MODE, K_SHIFT | K_F },
    /* 0x8B */ { "%\"", GR_MODE, K_QUOTE },
    /* 0x8C */ { "%#", GR_MODE, K_POUND },
    /* 0x8D */ { "%$", GR_MODE, K_DOLLAR },
    /* 0x8E */ { "%:", GR_MODE, K_COLON },
    /* 0x8F */ { "%?", GR_MODE, K_QUESTION },
    /* 0x90 */ { "%(", GR_MODE, K_LPAREN },
    /* 0x91 */ { "%)", GR_MODE, K_RPAREN },
    /* 0x92 */ { "%>", GR_MODE, K_GT },
    /* 0x93 */ { "%<", GR_MODE, K_LT },
    /* 0x94 */ { "%=", GR_MODE, K_EQ },
    /* 0x95 */ { "%+", GR_MODE, K_PLUS },
    /* 0x96 */ { "%-", GR_MODE, K_MINUS },
    /* 0x97 */ { "%*", GR_MODE, K_MULTIPLY },
    /* 0x98 */ { "%/", GR_MODE, K_DIVIDE },
    /* 0x99 */ { "%;", GR_MODE, K_SEMICOLON },
    /* 0x9A */ { "%,", GR_MODE, K_COMMA },
    /* 0x9B */ { "%.", GR_MODE, K_PERIOD },
    /* 0x9C */ { "%0", GR_MODE, K_0 },
    /* 0x9D */ { "%1", GR_MODE, K_1 },
    /* 0x9E */ { "%2", GR_MODE, K_2 },
    /* 0x9F */ { "%3", GR_MODE, K_3 },
    /* 0xA0 */ { "%4", GR_MODE, K_4 },
    /* 0xA1 */ { "%5", GR_MODE, K_5 },
    /* 0xA2 */ { "%6", GR_MODE, K_6 },
    /* 0xA3 */ { "%7", GR_MODE, K_7 },
    /* 0xA4 */ { "%8", GR_MODE, K_8 },
    /* 0xA5 */ { "%9", GR_MODE, K_9 },
    /* 0xA6 */ { "%A", GR_MODE, K_A },
    /* 0xA7 */ { "%B", GR_MODE, K_B },
    /* 0xA8 */ { "%C", GR_MODE, K_C },
    /* 0xA9 */ { "%D", GR_MODE, K_D },
    /* 0xAA */ { "%E", GR_MODE, K_E },
    /* 0xAB */ { "%F", GR_MODE, K_F },
    /* 0xAC */ { "%G", GR_MODE, K_G },
    /* 0xAD */ { "%H", GR_MODE, K_H },
    /* 0xAE */ { "%I", GR_MODE, K_I },
    /* 0xAF */ { "%J", GR_MODE, K_J },
    /* 0xB0 */ { "%K", GR_MODE, K_K },
    /* 0xB1 */ { "%L", GR_MODE, K_L },
    /* 0xB2 */ { "%M", GR_MODE, K_M },
    /* 0xB3 */ { "%N", GR_MODE, K_N },
    /* 0xB4 */ { "%O", GR_MODE, K_O },
    /* 0xB5 */ { "%P", GR_MODE, K_P },
    /* 0xB6 */ { "%Q", GR_MODE, K_Q },
    /* 0xB7 */ { "%R", GR_MODE, K_R },
    /* 0xB8 */ { "%S", GR_MODE, K_S },
    /* 0xB9 */ { "%T", GR_MODE, K_T },
    /* 0xBA */ { "%U", GR_MODE, K_U },
    /* 0xBB */ { "%V", GR_MODE, K_V },
    /* 0xBC */ { "%W", GR_MODE, K_W },
    /* 0xBD */ { "%X", GR_MODE, K_X },
    /* 0xBE */ { "%Y", GR_MODE, K_Y },
    /* 0xBF */ { "%Z", GR_MODE, K_Z },
    /* 0xC0 */ { "\\\"", KL_MODE, K_DQUOTE },
    /* 0xC1 */ { "AT", FN_MODE, FN_AT },
    /* 0xC2 */ { "TAB", FN_MODE, FN_TAB },
    /* 0xC3 */ { NULL, 0, 0 },
    /* 0xC4 */ { "CODE", FN_MODE, FN_CODE },
    /* 0xC5 */ { "VAL", FN_MODE, FN_VAL },
    /* 0xC6 */ { "LEN", FN_MODE, FN_LEN },
    /* 0xC7 */ { "SIN", FN_MODE, FN_SIN },
    /* 0xC8 */ { "COS", FN_MODE, FN_COS },
    /* 0xC9 */ { "TAN", FN_MODE, FN_TAN },
    /* 0xCA */ { "ASN", FN_MODE, FN_ARCSIN },
    /* 0xCB */ { "ACS", FN_MODE, FN_ARCCOS },
    /* 0xCC */ { "ATN", FN_MODE, FN_ARCTAN },
    /* 0xCD */ { "LN", FN_MODE, FN_LN },
    /* 0xCE */ { "EXP", FN_MODE, FN_EXP },
    /* 0xCF */ { "INT", FN_MODE, FN_INT },
    /* 0xD0 */ { "SQR", FN_MODE, FN_SQR },
    /* 0xD1 */ { "SGN", FN_MODE, FN_SGN },
    /* 0xD2 */ { "ABS", FN_MODE, FN_ABS },
    /* 0xD3 */ { "PEEK", FN_MODE, FN_PEEK },
    /* 0xD4 */ { "USR", FN_MODE, FN_USR },
    /* 0xD5 */ { "STR$", FN_MODE, FN_STR_DLR },
    /* 0xD6 */ { "CHR$", FN_MODE, FN_CHR_DLR },
    /* 0xD7 */ { "NOT", FN_MODE, FN_NOT },
    /* 0xD8 */ { "**", KL_MODE, K_POWER },
    /* 0xD9 */ { "OR", KL_MODE, K_OR },
    /* 0xDA */ { "AND", KL_MODE, K_AND },
    /* 0xDB */ { "<=", KL_MODE, K_LE },
    /* 0xDC */ { ">=", KL_MODE, K_GE },
    /* 0xDD */ { "<>", KL_MODE, K_NE },
    /* 0xDE */ { "THEN", KL_MODE, K_THEN },
    /* 0xDF */ { "TO", KL_MODE, K_TO },
    /* 0xE0 */ { "STEP", KL_MODE, K_STEP },
    /* 0xE1 */ { "LPRINT", KL_MODE, K_LPRINT },
    /* 0xE2 */ { "LLIST", KL_MODE, K_LLIST },
    /* 0xE3 */ { "STOP", KL_MODE, K_STOP },
    /* 0xE4 */ { "SLOW", KL_MODE, K_SLOW },
    /* 0xE5 */ { "FAST", KL_MODE, K_FAST },
    /* 0xE6 */ { "NEW", KL_MODE, KW_NEW },
    /* 0xE7 */ { "SCROLL", KL_MODE, KW_SCROLL },
    /* 0xE8 */ { "CONT", KL_MODE, KW_CONT },
    /* 0xE9 */ { "DIM", KL_MODE, KW_DIM },
    /* 0xEA */ { "REM", KL_MODE, KW_REM },
    /* 0xEB */ { "FOR", KL_MODE, KW_FOR },
    /* 0xEC */ { "GOTO", KL_MODE, KW_GOTO },
    /* 0xED */ { "GOSUB", KL_MODE, KW_GOSUB },
    /* 0xEE */ { "INPUT", KL_MODE, KW_INPUT },
    /* 0xEF */ { "LOAD", KL_MODE, KW_LOAD },
    /* 0xF0 */ { "LIST", KL_MODE, KW_LIST },
    /* 0xF1 */ { "LET", KL_MODE, KW_LET },
    /* 0xF2 */ { "PAUSE", KL_MODE, KW_PAUSE },
    /* 0xF3 */ { "NEXT", KL_MODE, KW_NEXT },
    /* 0xF4 */ { "POKE", KL_MODE, KW_POKE },
    /* 0xF5 */ { "PRINT", KL_MODE, KW_PRINT },
    /* 0xF6 */ { "PLOT", KL_MODE, KW_PLOT },
    /* 0xF7 */ { "RUN", KL_MODE, KW_RUN },
    /* 0xF8 */ { "SAVE", KL_MODE, KW_SAVE },
    /* 0xF9 */ { "RAND", KL_MODE, KW_RAND },
    /* 0xFA */ { "IF", KL_MODE, KW_IF },
    /* 0xFB */ { "CLS", KL_MODE, KW_CLS },
    /* 0xFC */ { "UNPLOT", KL_MODE, KW_UNPLOT },
    /* 0xFD */ { "CLEAR", KL_MODE, KW_CLEAR },
    /* 0xFE */ { "RETURN", KL_MODE, KW_RETURN },
    /* 0xFF */ { "COPY", KL_MODE, KW_COPY }
};

static const keyword_map_t* zx81_lookup_code(const char** p,
        bool allow_keyword) {
    if (p == NULL || *p == NULL) {
        return NULL;
    }

    for (int i = sizeof(keyword_map) / sizeof(keyword_map[0]) - 1; i >= 0; i--) {
        const char* keyword = keyword_map[i].keyword;
        if (keyword == NULL) {
            continue;
        }
        if (!allow_keyword && (i & 0x40) != 0) {
            continue; // skip function keywords if not allowed
        }

        if (keyword != NULL) {
            size_t len = strlen(keyword);
            if (strn_case_cmp(*p, keyword, len) == 0) {
                *p += len; // advance the pointer
                return &keyword_map[i];
            }
        }
    }
    return NULL;
}

static void push_one_token(const char** p, int* mode, bool allow_keywords) {
    if (**p == '\0') {
        return;
    }

    // lookup the keyword in the mapping table
    const keyword_map_t* kw_map = zx81_lookup_code(p, allow_keywords);
    if (kw_map == NULL) {
        printf("Error: Unknown keyword in text: %s\n", *p);
        (*p)++;
        return;
    }

    // change mode if needed
    switch (kw_map->mode) {
    case KL_MODE:
        if (*mode == GR_MODE) {
            kbd_push_key(K_GRAPHICS);
            *mode = KL_MODE;
        }
        break;
    case FN_MODE:
        kbd_push_key(K_FUNCTION);
        break;
    case GR_MODE:
        if (*mode != GR_MODE) {
            kbd_push_key(K_GRAPHICS);
            *mode = GR_MODE;
        }
        break;
    default:
        printf("Error: Invalid mode in keyword mapping: %d\n", kw_map->mode);
        break;
    }

    // send the key to the keyboard queue
    kbd_push_key(kw_map->key);
}

void basic_paste(const char* text) {
    const char* p = text;
    int mode = KL_MODE;
    while (*p != '\0') {
        if (*p == '\r' || *p == '\n') {
            kbd_push_key(K_RETURN);
            mode = KL_MODE;
            p++;
            continue;
        }

        if (isspace((unsigned char)*p)) {
            p++;
            continue;
        }

        if (*p == '"') {
            // extract quoted string from input
            UT_string* quoted_text;
            utstring_new(quoted_text);
            utstring_printf(quoted_text, "\"");
            p++;
            while (*p != '\0' && *p != '"' && *p != '\r' && *p != '\n') {
                utstring_printf(quoted_text, "%c", *p);
                p++;
            }
            if (*p == '"') {
                utstring_printf(quoted_text, "\"");
                p++;
            }

            // push the quoted string to the keyboard queue
            const char* quoted_str = utstring_body(quoted_text);
            while (*quoted_str != '\0') {
                if (*quoted_str == '\\' && *(quoted_str + 1) == '"') {
                    if (mode == GR_MODE) {
                        kbd_push_key(K_GRAPHICS); // return to KL_MODE after pasting
                        mode = KL_MODE;
                    }
                    kbd_push_key(K_DQUOTE);
                    quoted_str += 2; // skip the escaped quote
                }
                else {
                    push_one_token(&quoted_str, &mode, false);
                }
            }

            utstring_free(quoted_text);
            continue;
        }

        // allow keywords in BASIC code
        push_one_token(&p, &mode, true);
    }

    if (mode == GR_MODE) {
        kbd_push_key(K_GRAPHICS); // return to KL_MODE after pasting
    }
}
