//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#pragma once

#include "simz80.h"

void zx81screen_init(void);
void zx81screen_update(WORD charset_addr, WORD display_addr);
void zx81screen_free(void);
