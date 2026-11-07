//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#pragma once

#include "simz80.h"

// push a file to be loaded by LOAD ""
void push_load_file(const char* p_file);

// hook for NEXT_PROG, routine in LOAD after file name was parsed
void next_prog_hook(Z80* z80);

// save a program to a .p file, hook for SAVE1, routine in SAVE after file name was parsed
void save_prog_hook(Z80* z80);
