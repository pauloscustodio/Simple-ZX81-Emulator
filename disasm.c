//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "disasm.h"
#include "simz80.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

static const char* catf(const char* fmt, ...) {
    static char buffer[64];
    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    return buffer;
}

static const char* dd1(int n, int x) {
    static const char* n_lut[] = { "bc", "de", "hl", "sp" };
    static const char* x_lut[] = { "hl", "ix", "iy" };
    n &= 0x03;
    if (n == 2) {
        return x_lut[x];
    }
    else {
        return n_lut[n];
    }
}

static const char* dd2(int n, int x) {
    static const char* n_lut[] = { "bc", "de", "hl", "af" };
    static const char* x_lut[] = { "hl", "ix", "iy" };
    n &= 0x03;
    if (n == 2) {
        return x_lut[x];
    }
    else {
        return n_lut[n];
    }
}

static const char* r1(int n, int x, int dis) {
    static char buffer[16];
    static const char* n_lut[] = { "b", "c", "d", "e", "h", "l", "(hl)", "a" };
    static const char* x_lut[] = { "(hl)", "(ix%+02x)", "(iy%+02x)" };
    n &= 0x07;
    if (n == 6) {
        sprintf(buffer, x_lut[x], dis);
        return buffer;
    }
    else {
        return n_lut[n];
    }
}

static const char* x1(int x) {
    static const char* x_lut[] = { "hl", "ix", "iy" };
    return x_lut[x];
}

static const char* flags1(int n) {
    static const char* n_lut[] = { "nz", "z", "nc", "c", "po", "pe", "p", "m" };
    n &= 0x07;
    return n_lut[n];
}

static const char* flags2(int n) {
    n &= 0x03;
    return flags1(n);
}

static const char* alu1(int n) {
    static const char* n_lut[] = { "add a, ", "adc a, ", "sub ", "sbc a, ", "and ", "xor ", "or ", "cp " };
    n &= 0x07;
    return n_lut[n];
}

static const char* rot1(int n) {
    static const char* n_lut[] = { "rlc ", "rrc ", "rl ", "rr ", "sla ", "sra ", "sll ", "srl " };
    n &= 0x07;
    return n_lut[n];
}

static const char* bit1(int n) {
    static const char* n_lut[] = { "", "bit ", "res ", "set " };
    n &= 0x03;
    return n_lut[n];
}

static const char* collect_nn(Z80* z80, WORD* a) {
    static char buffer[16];
    WORD nn = z80_fetch(z80, *a) | (z80_fetch(z80, *a + 1) << 8);
    sprintf(buffer, "%04x", nn);
    (*a) += 2;
    return buffer;
}

static const char* collect_n(Z80* z80, WORD* a) {
    static char buffer[16];
    BYTE n = z80_fetch(z80, *a);
    sprintf(buffer, "%02x", n);
    (*a)++;
    return buffer;
}

static int collect_dis(Z80* z80, WORD* a) {
    int dis = (signed char)z80_fetch(z80, *a);
    (*a)++;
    return dis;
}

static const char* collect_jr(Z80* z80, WORD* a) {
    static char buffer[16];
    int dis = (signed char)z80_fetch(z80, *a);
    WORD addr = *a + 1 + dis;
    sprintf(buffer, "%04x", addr);
    (*a)++;
    return buffer;
}

static const char* disasm_str(Z80* z80, WORD* a) {
    int x = 0;		// hl, ix, iy
    int dis = 0;	// displacement for ix/iy
    bool done = false;
    while (!done) {
        done = true;
        int b = z80_fetch(z80, *a);
        (*a)++;
        switch (b) {
        case 0x00:
            return "nop";
            break;
        case 0x01:
        case 0x11:
        case 0x21:
        case 0x31:
            return catf("ld %s, %s", dd1(b >> 4, x), collect_nn(z80, a));
            break;
        case 0x02:
        case 0x12:
            return catf("ld (%s), a", dd1(b >> 4, x));
            break;
        case 0x03:
        case 0x13:
        case 0x23:
        case 0x33:
            return catf("inc %s", dd1(b >> 4, x));
            break;
        case 0x04:
        case 0x0c:
        case 0x14:
        case 0x1c:
        case 0x24:
        case 0x2c:
        case 0x34:
        case 0x3c:
            if (b == 0x34 && x != 0) {
                dis = collect_dis(z80, a);
            }
            return catf("inc %s", r1(b >> 3, x, dis));
        case 0x05:
        case 0x0d:
        case 0x15:
        case 0x1d:
        case 0x25:
        case 0x2d:
        case 0x35:
        case 0x3d:
            if (b == 0x35 && x != 0) {
                dis = collect_dis(z80, a);
            }
            return catf("dec %s", r1(b >> 3, x, dis));
        case 0x06:
        case 0x0e:
        case 0x16:
        case 0x1e:
        case 0x26:
        case 0x2e:
        case 0x36:
        case 0x3e:
            if (b == 0x36 && x != 0) {
                dis = collect_dis(z80, a);
            }
            return catf("ld %s, %s", r1(b >> 3, x, dis), collect_n(z80, a));
        case 0x07:
            return "rlca";
        case 0x08:
            return "ex af, af'";

        case 0x09:
        case 0x19:
        case 0x29:
        case 0x39:
            return catf("add %s, %s", x1(x), dd1(b >> 4, x));
        case 0x0a:
        case 0x1a:
            return catf("ld a, (%s)", dd1(b >> 4, 0));
            break;
        case 0x0b:
        case 0x1b:
        case 0x2b:
        case 0x3b:
            return catf("dec %s", dd1(b >> 4, x));
        case 0x0f:
            return "rrca";
        case 0x10:
            return catf("djnz %s", collect_jr(z80, a));
        case 0x17:
            return "rla";
        case 0x18:
            return catf("jr %s", collect_jr(z80, a));
        case 0x1f:
            return "rra";
        case 0x20:
        case 0x28:
        case 0x30:
        case 0x38:
            return catf("jr %s, %s", flags2(b >> 3), collect_jr(z80, a));
        case 0x22:
            return catf("ld (%s), %s", collect_nn(z80, a), x1(x));
        case 0x27:
            return "daa";
        case 0x2a:
            return catf("ld %s, (%s)", x1(x), collect_nn(z80, a));
        case 0x2f:
            return "cpl";
            break;
        case 0x32:
            return catf("ld (%s), a", collect_nn(z80, a));
        case 0x37:
            return "scf";
        case 0x3a:
            return catf("ld a, (%s)", collect_nn(z80, a));
        case 0x3f:
            return "ccf";
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            if (x != 0)
                if ((b & 0x07) == 0x06 || (b & 0x38) == 0x30) {
                    dis = collect_dis(z80, a);
                }
            return catf("ld %s, %s", r1(b >> 3, x, dis), r1(b, x, dis));
        case 0x76:
            return "halt";
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87:
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        case 0x8d:
        case 0x8e:
        case 0x8f:
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97:
        case 0x98:
        case 0x99:
        case 0x9a:
        case 0x9b:
        case 0x9c:
        case 0x9d:
        case 0x9e:
        case 0x9f:
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xa7:
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
        case 0xac:
        case 0xad:
        case 0xae:
        case 0xaf:
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
            if (x != 0)
                if ((b & 0x07) == 0x06) {
                    dis = collect_dis(z80, a);
                }
            return catf("%s%s", alu1(b >> 3), r1(b, x, dis));
        case 0xc0:
        case 0xc8:
        case 0xd0:
        case 0xd8:
        case 0xe0:
        case 0xe8:
        case 0xf0:
        case 0xf8:
            return catf("ret %s", flags1(b >> 3));
        case 0xc1:
        case 0xd1:
        case 0xe1:
        case 0xf1:
            return catf("pop %s", dd2(b >> 4, x));
        case 0xc2:
        case 0xca:
        case 0xd2:
        case 0xda:
        case 0xe2:
        case 0xea:
        case 0xf2:
        case 0xfa:
            return catf("jp %s, %s", flags1(b >> 3), collect_nn(z80, a));
        case 0xc3:
            return catf("jp %s", collect_nn(z80, a));
        case 0xc4:
        case 0xcc:
        case 0xd4:
        case 0xdc:
        case 0xe4:
        case 0xec:
        case 0xf4:
        case 0xfc:
            return catf("call %s, %s", flags1(b >> 3), collect_nn(z80, a));
        case 0xc5:
        case 0xd5:
        case 0xe5:
        case 0xf5:
            return catf("push %s", dd2(b >> 4, x));
        case 0xc6:
        case 0xce:
        case 0xd6:
        case 0xde:
        case 0xe6:
        case 0xee:
        case 0xf6:
        case 0xfe:
            return catf("%s%s", alu1(b >> 3), collect_n(z80, a));
        case 0xc7:
        case 0xcf:
        case 0xd7:
        case 0xdf:
        case 0xe7:
        case 0xef:
        case 0xf7:
        case 0xff:
            return catf("rst %02xh", b & 0x34);
        case 0xc9:
            return "ret";
        case 0xcb:
            if (x != 0) {
                dis = collect_dis(z80, a);
            }
            b = z80_fetch(z80, *a);
            (*a)++;
            if (b <= 0x3f) {
                return catf("%s%s", rot1(b >> 3), r1(b, x, dis));
            }
            else {
                return catf("%s%d, %s", bit1(b >> 6), (b >> 3) & 0x07, r1(b, x, dis));
            }
        case 0xcd:
            return catf("call %s", collect_nn(z80, a));
        case 0xd3:
            return catf("out (%s), a", collect_n(z80, a));
        case 0xd9:
            return "exx";
        case 0xdb:
            return catf("in a, (%s)", collect_n(z80, a));
        case 0xdd:
            x = 1;
            done = false;
            break;
        case 0xe3:
            return catf("ex (sp), %s", x1(x));
        case 0xe9:
            return catf("jp (%s)", x1(x));
        case 0xeb:
            return catf("ex de, %s", x1(x));
        case 0xed:
            b = z80_fetch(z80, *a);
            (*a)++;
            switch (b) {
            case 0x40:
            case 0x48:
            case 0x50:
            case 0x58:
            case 0x60:
            case 0x68:
            case 0x78:
                return catf("in %s, (c)", r1(b >> 3, 0, 0));
            case 0x41:
            case 0x49:
            case 0x51:
            case 0x59:
            case 0x61:
            case 0x69:
            case 0x79:
                return catf("out (c), %s", r1(b >> 3, 0, 0));
            case 0x42:
            case 0x52:
            case 0x62:
            case 0x72:
                return catf("sbc hl, %s", dd1(b >> 4, 0));
            case 0x43:
            case 0x53:
            case 0x63:
            case 0x73:
                return catf("ld (%s), %s", collect_nn(z80, a), dd1(b >> 4, 0));
            case 0x44:
                return "neg";
            case 0x45:
                return "retn";
            case 0x46:
                return "im 0";
            case 0x47:
                return "ld i, a";
            case 0x4a:
            case 0x5a:
            case 0x6a:
            case 0x7a:
                return catf("adc hl, %s", dd1(b >> 4, 0));
            case 0x4b:
            case 0x5b:
            case 0x6b:
            case 0x7b:
                return catf("ld %s, (%s)", dd1(b >> 4, 0), collect_nn(z80, a));
            case 0x4d:
                return "reti";
            case 0x4f:
                return "ld r, a";
            case 0x56:
                return "im 1";
            case 0x57:
                return "ld a, i";
            case 0x5e:
                return "im 2";
            case 0x5f:
                return "ld a, r";
            case 0x67:
                return "rrd";
            case 0x6f:
                return "rld";
            case 0xa0:
                return "ldi";
            case 0xa1:
                return "cpi";
            case 0xa2:
                return "ini";
            case 0xa3:
                return "outi";
            case 0xa8:
                return "ldd";
            case 0xa9:
                return "cpd";
            case 0xaa:
                return "ind";
            case 0xab:
                return "outd";
            case 0xb0:
                return "ldir";
            case 0xb1:
                return "cpir";
            case 0xb2:
                return "inir";
            case 0xb3:
                return "otir";
            case 0xb8:
                return "lddr";
            case 0xb9:
                return "cpdr";
            case 0xba:
                return "indr";
            case 0xbb:
                return "otdr";
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x1a:
            case 0x1b:
            case 0x1c:
            case 0x1d:
            case 0x1e:
            case 0x1f:
            case 0x20:
            case 0x21:
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x2c:
            case 0x2d:
            case 0x2e:
            case 0x2f:
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x3f:
            case 0x4c:
            case 0x4e:
            case 0x54:
            case 0x55:
            case 0x5c:
            case 0x5d:
            case 0x64:
            case 0x65:
            case 0x66:
            case 0x6c:
            case 0x6d:
            case 0x6e:
            case 0x70:
            case 0x71:
            case 0x74:
            case 0x75:
            case 0x76:
            case 0x77:
            case 0x7c:
            case 0x7d:
            case 0x7e:
            case 0x7f:
            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
            case 0x8a:
            case 0x8b:
            case 0x8c:
            case 0x8d:
            case 0x8e:
            case 0x8f:
            case 0x90:
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x96:
            case 0x97:
            case 0x98:
            case 0x99:
            case 0x9a:
            case 0x9b:
            case 0x9c:
            case 0x9d:
            case 0x9e:
            case 0x9f:
            case 0xa4:
            case 0xa5:
            case 0xa6:
            case 0xa7:
            case 0xac:
            case 0xad:
            case 0xae:
            case 0xaf:
            case 0xb4:
            case 0xb5:
            case 0xb6:
            case 0xb7:
            case 0xbc:
            case 0xbd:
            case 0xbe:
            case 0xbf:
            case 0xc0:
            case 0xc1:
            case 0xc2:
            case 0xc3:
            case 0xc4:
            case 0xc5:
            case 0xc6:
            case 0xc7:
            case 0xc8:
            case 0xc9:
            case 0xca:
            case 0xcb:
            case 0xcc:
            case 0xcd:
            case 0xce:
            case 0xcf:
            case 0xd0:
            case 0xd1:
            case 0xd2:
            case 0xd3:
            case 0xd4:
            case 0xd5:
            case 0xd6:
            case 0xd7:
            case 0xd8:
            case 0xd9:
            case 0xda:
            case 0xdb:
            case 0xdc:
            case 0xdd:
            case 0xde:
            case 0xdf:
            case 0xe0:
            case 0xe1:
            case 0xe2:
            case 0xe3:
            case 0xe4:
            case 0xe5:
            case 0xe6:
            case 0xe7:
            case 0xe8:
            case 0xe9:
            case 0xea:
            case 0xeb:
            case 0xec:
            case 0xed:
            case 0xee:
            case 0xef:
            case 0xf0:
            case 0xf1:
            case 0xf2:
            case 0xf3:
            case 0xf4:
            case 0xf5:
            case 0xf6:
            case 0xf7:
            case 0xf8:
            case 0xf9:
            case 0xfa:
            case 0xfb:
            case 0xfc:
            case 0xfd:
            case 0xfe:
            case 0xff:
                return catf("defb %s", collect_n(z80, a));
            }
            break;
        case 0xf3:
            return "di";
        case 0xf9:
            return catf("ld sp, %s", x1(x));
        case 0xfb:
            return "ei";
        case 0xfd:
            x = 2;
            done = false;
            break;
        }
    }
    return "???";
}

void disasm(Z80* z80, WORD a) {
    WORD a0 = a;
    const char* asm_str = disasm_str(z80, &a);

    printf("%04x  ", a0);
    for (WORD i = a0; i != a; i++) {
        printf("%02x ", z80_fetch(z80, i));
    }
    for (WORD i = a; i != (WORD)(a0 + 4); i++) {
        printf("   ");
    }
    printf(" %s\n", asm_str);
}

static const char* flags_str(WORD af) {
    static char str[9];
    str[0] = (af & 0x80) ? 'S' : '.';
    str[1] = (af & 0x40) ? 'Z' : '.';
    str[2] = (af & 0x20) ? '5' : '.';
    str[3] = (af & 0x10) ? 'H' : '.';
    str[4] = (af & 0x08) ? '3' : '.';
    str[5] = (af & 0x04) ? 'P' : '.';
    str[6] = (af & 0x02) ? 'N' : '.';
    str[7] = (af & 0x01) ? 'C' : '.';
    str[8] = '\0';
    return str;
}

void show_regs(Z80* z80) {
    printf("AF = %04x %s  BC = %04x DE = %04x HL = %04x\n",
           z80->af[z80->af_sel], flags_str(z80->af[z80->af_sel]),
           z80->regs[z80->regs_sel].bc, z80->regs[z80->regs_sel].de,
           z80->regs[z80->regs_sel].hl);
    printf("AF'= %04x %s  BC'= %04x DE'= %04x HL'= %04x\n",
           z80->af[1 - z80->af_sel], flags_str(z80->af[1 - z80->af_sel]),
           z80->regs[1 - z80->regs_sel].bc,
           z80->regs[1 - z80->regs_sel].de, z80->regs[1 - z80->regs_sel].hl);
    printf("IR = %04x IX = %04x IY = %04x SP = %04x PC = %04x\n",
           z80->ir, z80->ix, z80->iy, z80->sp, z80->pc);
}
