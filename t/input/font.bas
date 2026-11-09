#ASM
font_ori equ	0x1e00
font_dst equ	0x2000

font_src:
#include <bold_font.asm>
font_end:

copy_font:
		ld  	hl, font_src
		ld 		de, font_dst
		ld  	bc, font_end-font_src
		ldir
		ret

new_font:
		ld 		a, font_dst >> 8
		ld 		i, a
		ret

old_font:
		ld 		a, font_ori >> 8
		ld 		i, a
		ret

#ENDASM

	RAND USR &copy_font
	FOR I=0 TO 255
		PRINT CHR$(I);
	NEXT I
	
	FOR I=1 TO 10
		RAND USR &new_font
		PAUSE 50
		RAND USR &old_font
		PAUSE 50
	NEXT I

	RAND USR 8191

#AUTOSTART=1
