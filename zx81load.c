//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "utils.h"
#include "utstring.h"
#include "zx81.h"
#include "zx81chars.h"
#include "zx81kbd.h"
#include "zx81load.h"
#include <stdio.h>
#include <string.h>

static char* pending_load_file = NULL;

void push_load_file(const char* p_file) {
    if (pending_load_file) {
        free(pending_load_file);
    }

    pending_load_file = xstrdup(p_file);
    kbd_push_key(KW_LOAD);
    kbd_push_key(K_QUOTE);
    kbd_push_key(K_QUOTE);
    kbd_push_key(K_RETURN);
}

void next_prog_hook(Z80* z80) {
    WORD de = z80->regs[z80->regs_sel].de;
    if (pending_load_file == NULL || (de & 0x8000) == 0) {
        // nothing to load, or LOAD "FILE"
        z80->pc = RET_ADDR;
    }

    // find the size of the .p file
    struct stat st;
    xstat(pending_load_file, &st);

    // load the .p file into memory starting at address SAVE_ADDR
    FILE* fp = xfopen(pending_load_file, "rb");
    xfread(&memory[SAVE_ADDR], 1, st.st_size, fp);
    xfclose(fp);

    // cancel load for next time
    free(pending_load_file);
    pending_load_file = NULL;

    // jump to RET
    z80->pc = RET_ADDR;
}

void save_prog_hook(Z80* z80) {
    UT_string* filename;
    utstring_new(filename);

    // collect file name
    WORD hl = z80->regs[z80->regs_sel].hl;  // address of the program to save
    for (int i = 0; ; i++) {
        BYTE b = PEEK(hl + i);
        utstring_printf(filename, "%s",
                        zx81chars[b & 0x7f]);  // convert to string using zx81chars
        if ((b & 0x80) != 0) {
            break;
        }
    }
    str_tolower(utstring_body(filename));
    utstring_printf(filename, ".p");

    // save the program to a .p file
    size_t size = PEEKW(E_LINE) - SAVE_ADDR;
    FILE* fp = xfopen(utstring_body(filename), "wb");
    xfwrite(&memory[SAVE_ADDR], 1, size, fp);
    xfclose(fp);

    // jump to RET
    z80->pc = RET_ADDR;

    utstring_free(filename);
}
