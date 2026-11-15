#AUTOSTART=1
#ASM

KEYBOARD equ $02bb
DECODE   equ $07bd
D_FILE   equ $400c
LAST_K   equ $4025
FRAMES   equ $4034

show_keys:
; read the keyboard
#ifdef ROM
        call KEYBOARD
#else
        ld hl,(LAST_K)          ; HL=key pressed
#endif
        push hl                 ; save key pressed

; show key mask on screen
        ld de, (D_FILE)
        inc de

; show high byte of key mask
        ld b,8
h_digits:        
        rl h
        call print_carry
        djnz h_digits

        xor a
        ld (de), a
        inc de

; show low byte of key mask
        ld b,8
l_digits:        
        rl l
        call print_carry
        djnz l_digits

; output a space and delete previous key
        xor a                   ; output a space
        ld (de), a
        inc de
        ld (de), a              ; delete previous key

        pop bc                  ; BC = key pressed

; check if any key pressed
; DECODE loops forever if B=$FF or C=$FF
        ld a, b
        inc a
        ret z
        ld a, c
        inc a
        ret z

; decode the key and show it on the screen
        push de                 ; save screen address

        call DECODE             ; in: BC=key pressed, out: HL=address of key
        pop de                  ; screen address
        ret nc                  ; no key pressed

        ld a, (hl)              ; get key pressed
        ld (de), a              ; print key pressed
        ret

print_carry:
        ld a,$1C                ; '0'
        jr nc, not_zero
        inc a                   ; '1' 
not_zero:
        ld (de),a               ; print digit
        inc de
        ret
#ENDASM

	LET start=PEEK &FRAMES+256*PEEK(&FRAMES+1)
@loop:
	RAND USR &show_keys
	LET end=PEEK &FRAMES+256*PEEK(&FRAMES+1)
	IF start-end<500 THEN GOTO @loop
        RAND USR 8191

