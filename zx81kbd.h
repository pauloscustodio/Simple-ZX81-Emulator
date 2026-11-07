//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#pragma once

#include "simz80.h"
#include <stdint.h>
#include <SDL.h>

// keyboard queue size and number of reads a key is kept pressed
#define KBD_QUEUE_SIZE 256
#define KBD_READ_COUNT 100

// bit mask with 1 at each selected column of the 8 keyboard rows
typedef uint64_t kbd_state_t;

// keyboard masks
#define K_SHIFT  	( 1ull << (0 * 8))
#define K_Z      	( 2ull << (0 * 8))
#define K_X      	( 4ull << (0 * 8))
#define K_C      	( 8ull << (0 * 8))
#define K_V      	(16ull << (0 * 8))
#define K_A      	( 1ull << (1 * 8))
#define K_S      	( 2ull << (1 * 8))
#define K_D      	( 4ull << (1 * 8))
#define K_F      	( 8ull << (1 * 8))
#define K_G      	(16ull << (1 * 8))
#define K_Q      	( 1ull << (2 * 8))
#define K_W      	( 2ull << (2 * 8))
#define K_E      	( 4ull << (2 * 8))
#define K_R      	( 8ull << (2 * 8))
#define K_T      	(16ull << (2 * 8))
#define K_1      	( 1ull << (3 * 8))
#define K_2      	( 2ull << (3 * 8))
#define K_3      	( 4ull << (3 * 8))
#define K_4      	( 8ull << (3 * 8))
#define K_5      	(16ull << (3 * 8))
#define K_0      	( 1ull << (4 * 8))
#define K_9      	( 2ull << (4 * 8))
#define K_8      	( 4ull << (4 * 8))
#define K_7      	( 8ull << (4 * 8))
#define K_6      	(16ull << (4 * 8))
#define K_P      	( 1ull << (5 * 8))
#define K_O      	( 2ull << (5 * 8))
#define K_I      	( 4ull << (5 * 8))
#define K_U      	( 8ull << (5 * 8))
#define K_Y      	(16ull << (5 * 8))
#define K_RETURN 	( 1ull << (6 * 8))
#define K_L      	( 2ull << (6 * 8))
#define K_K      	( 4ull << (6 * 8))
#define K_J      	( 8ull << (6 * 8))
#define K_H      	(16ull << (6 * 8))
#define K_SPACE  	( 1ull << (7 * 8))
#define K_PERIOD    ( 2ull << (7 * 8))
#define K_M      	( 4ull << (7 * 8))
#define K_N      	( 8ull << (7 * 8))
#define K_B      	(16ull << (7 * 8))

#define K_EDIT		(K_SHIFT | K_1)
#define K_AND		(K_SHIFT | K_2)
#define K_THEN		(K_SHIFT | K_3)
#define K_TO		(K_SHIFT | K_4)
#define K_LEFT		(K_SHIFT | K_5)
#define K_DOWN		(K_SHIFT | K_6)
#define K_UP		(K_SHIFT | K_7)
#define K_RIGHT		(K_SHIFT | K_8)
#define K_GRAPHICS	(K_SHIFT | K_9)
#define K_BACKSPACE	(K_SHIFT | K_0)
#define K_DQUOTE	(K_SHIFT | K_Q)
#define K_OR		(K_SHIFT | K_W)
#define K_STEP		(K_SHIFT | K_E)
#define K_LE		(K_SHIFT | K_R)
#define K_NE		(K_SHIFT | K_T)
#define K_GE		(K_SHIFT | K_Y)
#define K_DOLLAR	(K_SHIFT | K_U)
#define K_LPAREN	(K_SHIFT | K_I)
#define K_RPAREN	(K_SHIFT | K_O)
#define K_QUOTE		(K_SHIFT | K_P)
#define K_STOP		(K_SHIFT | K_A)
#define K_LPRINT	(K_SHIFT | K_S)
#define K_SLOW		(K_SHIFT | K_D)
#define K_FAST		(K_SHIFT | K_F)
#define K_LLIST		(K_SHIFT | K_G)
#define K_POWER		(K_SHIFT | K_H)
#define K_MINUS		(K_SHIFT | K_J)
#define K_PLUS		(K_SHIFT | K_K)
#define K_EQ		(K_SHIFT | K_L)
#define K_FUNCTION	(K_SHIFT | K_RETURN)
#define K_COLON		(K_SHIFT | K_Z)
#define K_SEMICOLON	(K_SHIFT | K_X)
#define K_QUESTION	(K_SHIFT | K_C)
#define K_DIVIDE	(K_SHIFT | K_V)
#define K_MULTIPLY	(K_SHIFT | K_B)
#define K_LT		(K_SHIFT | K_N)
#define K_GT		(K_SHIFT | K_M)
#define K_COMMA		(K_SHIFT | K_PERIOD)
#define K_POUND		(K_SHIFT | K_SPACE)

#define KW_PLOT		(K_Q)
#define KW_UNPLOT	(K_W)
#define KW_REM		(K_E)
#define KW_RUN		(K_R)
#define KW_RAND		(K_T)
#define KW_RETURN	(K_Y)
#define KW_IF		(K_U)
#define KW_INPUT	(K_I)
#define KW_POKE		(K_O)
#define KW_PRINT	(K_P)
#define KW_NEW		(K_A)
#define KW_SAVE		(K_S)
#define KW_DIM		(K_D)
#define KW_FOR		(K_F)
#define KW_GOTO		(K_G)
#define KW_GOSUB	(K_H)
#define KW_LOAD		(K_J)
#define KW_LIST		(K_K)
#define KW_LET		(K_L)
#define KW_COPY		(K_Z)
#define KW_CLEAR	(K_X)
#define KW_CONT		(K_C)
#define KW_CLS		(K_V)
#define KW_SCROLL	(K_B)
#define KW_NEXT		(K_N)
#define KW_PAUSE	(K_M)

#define FN_SIN		(K_Q)
#define FN_COS		(K_W)
#define FN_TAN		(K_E)
#define FN_INT		(K_R)
#define FN_RND		(K_T)
#define FN_STR_DLR	(K_Y)
#define FN_CHR_DLR	(K_U)
#define FN_CODE		(K_I)
#define FN_PEEK		(K_O)
#define FN_TAB		(K_P)
#define FN_ARCSIN	(K_A)
#define FN_ARCCOS	(K_S)
#define FN_ARCTAN	(K_D)
#define FN_SGN		(K_F)
#define FN_ABS		(K_G)
#define FN_SQR		(K_H)
#define FN_VAL		(K_J)
#define FN_LEN		(K_K)
#define FN_USR		(K_L)
#define FN_LN		(K_Z)
#define FN_EXP		(K_X)
#define FN_AT		(K_C)
#define FN_INKEY_DLR (K_B)
#define FN_NOT		(K_N)
#define FN_PI		(K_M)

// initialize the keyboard service and keyboard mappings
void kbd_init(void);

// interrupt service routine for keyboard scan
BYTE kbd_input_isr(Z80* z80, WORD addr);

// handle a keyboard event returned by SDL_PollEvent
void kbd_handle_sdl_event(const SDL_Event* event);

// push a key to the keyboard queue
void kbd_push_key(kbd_state_t key);
