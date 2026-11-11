//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "simz80.h"
#include "utils.h"
#include "utstring.h"
#include "zx81.h"
#include "zx81chars.h"
#include "zx81kbd.h"
#include "zx81load.h"
#include <stdio.h>
#include <sys/stat.h>

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

static UT_string* collect_file_name(WORD addr) {
    UT_string* filename;
    utstring_new(filename);

    // collect file name
    for (int i = 0; ; i++) {
        BYTE b = PEEK(addr + i);
        utstring_printf(filename, "%s",
                        zx81chars[b & 0x7f]);  // convert to string using zx81chars
        if ((b & 0x80) != 0) {
            break;
        }
    }
    str_tolower(utstring_body(filename));
    utstring_printf(filename, ".p");
    return filename;
}

static void load_p_file(const char* p_file) {
    // find the size of the .p file
    struct stat st;
    xstat(p_file, &st);

    // load the .p file into memory starting at address SAVE_ADDR
    FILE* fp = xfopen(p_file, "rb");
    xfread(&memory[SAVE_ADDR], 1, st.st_size, fp);
    xfclose(fp);
}

static void save_p_file(const char* p_file, WORD start_addr, WORD end_addr) {
    // save the program to a .p file
    size_t size = end_addr - start_addr;
    FILE* fp = xfopen(p_file, "wb");
    xfwrite(&memory[start_addr], 1, size, fp);
    xfclose(fp);
}

void next_prog_hook(Z80* z80) {
    WORD de = z80->regs[z80->regs_sel].de;
    bool is_load_qq = (de & 0x8000) != 0;

    if (pending_load_file) {
        // load file from command line
        load_p_file(pending_load_file);

        // cancel load for next time
        free(pending_load_file);
        pending_load_file = NULL;
    }
    else if (!is_load_qq) {
        // LOAD "FILE"
        UT_string* filename = collect_file_name(de);
        load_p_file(utstring_body(filename));
        utstring_free(filename);
    }
    else {
        // ignore LOAD ""
    }

    // jump to RET
    z80->pc = RET_ADDR;
}

void save_prog_hook(Z80* z80) {
    // collect file name
    WORD hl = z80->regs[z80->regs_sel].hl;  // address of the program to save

    // save the program to a .p file
    UT_string* filename = collect_file_name(hl);
    save_prog(utstring_body(filename));
    utstring_free(filename);

    // jump to RET
    z80->pc = RET_ADDR;
}

void load_prog(const char* p_file) {
    load_p_file(p_file);
}

void save_prog(const char* p_file) {
    save_p_file(p_file, SAVE_ADDR, PEEKW(E_LINE));
}

void write_state(const char* filename, Z80* z80, BYTE* memory) {
    BYTE regs[REGS_SIZE] = { 0 };

    FILE* fp = xfopen(filename, "wb");

    // write memory
    xfwrite(memory, 1, MEMSIZE, fp);

    BYTE* p = regs;
    // write AF BC DE HL
    WORD af = z80->af[z80->af_sel];
    *p++ = af & 0xff;
    *p++ = (af >> 8) & 0xff;
    WORD bc = z80->regs[z80->regs_sel].bc;
    *p++ = bc & 0xff;
    *p++ = (bc >> 8) & 0xff;
    WORD de = z80->regs[z80->regs_sel].de;
    *p++ = de & 0xff;
    *p++ = (de >> 8) & 0xff;
    WORD hl = z80->regs[z80->regs_sel].hl;
    *p++ = hl & 0xff;
    *p++ = (hl >> 8) & 0xff;

    // write AF' BC' DE' HL'
    WORD af_ = z80->af[1 - z80->af_sel];
    *p++ = af_ & 0xff;
    *p++ = (af_ >> 8) & 0xff;
    WORD bc_ = z80->regs[1 - z80->regs_sel].bc;
    *p++ = bc_ & 0xff;
    *p++ = (bc_ >> 8) & 0xff;
    WORD de_ = z80->regs[1 - z80->regs_sel].de;
    *p++ = de_ & 0xff;
    *p++ = (de_ >> 8) & 0xff;
    WORD hl_ = z80->regs[1 - z80->regs_sel].hl;
    *p++ = hl_ & 0xff;
    *p++ = (hl_ >> 8) & 0xff;

    // Write IR IX IY SP PC IFF
    WORD ir = z80->ir;
    *p++ = ir & 0xff;
    *p++ = (ir >> 8) & 0xff;
    WORD ix = z80->ix;
    *p++ = ix & 0xff;
    *p++ = (ix >> 8) & 0xff;
    WORD iy = z80->iy;
    *p++ = iy & 0xff;
    *p++ = (iy >> 8) & 0xff;
    WORD sp = z80->sp;
    *p++ = sp & 0xff;
    *p++ = (sp >> 8) & 0xff;
    WORD pc = z80->pc;
    *p++ = pc & 0xff;
    *p++ = (pc >> 8) & 0xff;
    WORD iff = z80->iff;
    *p++ = iff & 0xff;
    *p++ = (iff >> 8) & 0xff;

    size_t size = p - regs;
    xfwrite(regs, size, 1, fp);

    xfclose(fp);
}

void read_state(const char* filename, Z80* z80, BYTE* memory) {
    BYTE regs[REGS_SIZE] = { 0 };

    FILE* fp = xfopen(filename, "rb");

    // read memory
    xfread(memory, 1, MEMSIZE, fp);

    // read registers
    xfread(regs, REGS_SIZE, 1, fp);

    // load registers into z80
    BYTE* p = regs;
    // load AF BC DE HL
    WORD af = p[0] | p[1] << 8;
    p += 2;
    z80->af[z80->af_sel] = af;
    WORD bc = p[0] | p[1] << 8;
    p += 2;
    z80->regs[z80->regs_sel].bc = bc;
    WORD de = p[0] | p[1] << 8;
    p += 2;
    z80->regs[z80->regs_sel].de = de;
    WORD hl = p[0] | p[1] << 8;
    p += 2;
    z80->regs[z80->regs_sel].hl = hl;
    // load AF' BC' DE' HL'
    WORD af_ = p[0] | p[1] << 8;
    p += 2;
    z80->af[1 - z80->af_sel] = af_;
    WORD bc_ = p[0] | p[1] << 8;
    p += 2;
    z80->regs[1 - z80->regs_sel].bc = bc_;
    WORD de_ = p[0] | p[1] << 8;
    p += 2;
    z80->regs[1 - z80->regs_sel].de = de_;
    WORD hl_ = p[0] | p[1] << 8;
    p += 2;
    z80->regs[1 - z80->regs_sel].hl = hl_;
    // load IR IX IY SP PC IFF
    WORD ir = p[0] | p[1] << 8;
    p += 2;
    z80->ir = ir;
    WORD ix = p[0] | p[1] << 8;
    p += 2;
    z80->ix = ix;
    WORD iy = p[0] | p[1] << 8;
    p += 2;
    z80->iy = iy;
    WORD sp = p[0] | p[1] << 8;
    p += 2;
    z80->sp = sp;
    WORD pc = p[0] | p[1] << 8;
    p += 2;
    z80->pc = pc;
    WORD iff = p[0] | p[1] << 8;
    p += 2;
    z80->iff = iff;

    xfclose(fp);
}
