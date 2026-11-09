#ASM
font_ori equ	0x1e00
font_dst equ	0x2000
D_FILE  equ     0x400c

font_src:
#include <normal_font.asm>
#include <bold_font.asm>
font_end:

copy_font:
		ld  	hl, font_src
		ld 		de, font_dst
		ld  	bc, font_end-font_src
		ldir
		ret

show_chars:
		ld  	hl, (D_FILE)
		ld  	b, 0
loop:	
		inc  	hl
		ld  	a, (hl)
		cp 		0x76
		jr 		z, loop

		ld  	a, b
		and 	~0x40 & 0xff
		ld  	(hl), a

		inc  	b
		jr  	nz, loop

		ret

new_font:
		ld 		a, +(font_dst >> 8) + 1
		ld 		i, a
		ret

old_font:
		ld 		a, font_ori >> 8
		ld 		i, a
		ret

#ENDASM

	RAND USR &copy_font
	RAND USR &show_chars
	FOR I=1 TO 5
		RAND USR &new_font
		PAUSE 50
		RAND USR &old_font
		PAUSE 50
	NEXT I

	RAND USR 8191

#AUTOSTART=1
