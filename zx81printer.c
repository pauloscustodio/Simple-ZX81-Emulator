//-----------------------------------------------------------------------------
// Simple-ZX81-Emulator
// Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
// Modifications Copyright (C) Paulo Custodio, 2026
// License: GPL
//-----------------------------------------------------------------------------

#include "utils.h"
#include "zx81.h"
#include "zx81chars.h"
#include "zx81printer.h"
#include <stdio.h>
#include <stdlib.h>

static FILE* printer_file = NULL;

static void lprint_close(void) {
	if (printer_file) {
		fclose(printer_file);
		printer_file = NULL;
	}
}

static void lprint_open(void) {
	if (!printer_file) {
		printer_file = xfopen(LPRINT_NAME, "ab");
		atexit(lprint_close);
	}
}

void lprint_char(BYTE ch) {
	lprint_open();
	if (ch == 0x76) {
		fprintf(printer_file, "\n");
	}
	else {
		fprintf(printer_file, "%s", zx81chars[ch]);
	}
}

void copy() {
	lprint_open();
	WORD addr = PEEKW(D_FILE)+1;
	for (int row=0; row < NUM_ROWS; row++) {
		for (int col=0; col < NUM_COLS+1; col++) {
			BYTE ch = PEEK(addr++);
			lprint_char(ch);
			if (ch == 0x76) {
				break;
			}
		}
	}
}
