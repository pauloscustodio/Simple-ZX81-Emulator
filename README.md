# Simple - but understandable ZX81 emulator from:

https://leiradel.github.io/2011/06/11/How-to-Emulate.html

Released under GPL. 
- Some alteration Dec. 2025 - P. Sieg
- New development started Nov. 2026 by Paulo Custodio.
  Objective is to have a zx81 emulator that starts immediately and that
  I can use as test backend for my ZX81 decompiler https://github.com/pauloscustodio/zx81-decompile
  Today I'm using EightyOne, but it has too many features I don't need and takes
  about 20 seconds to start the emulated program.
- Improvements out of scope: hardware emulation of the display system, so only
  the default text based screen can be emulated. 

# Features
- Simple save (F6) and restore (F7) complete 64k memory to/from file memory.img
- Simple save (F8) and restore (F9) 16k memory chunk to/from file nnn.16k
  (nnn = value at memory location 16507; so POKE 16507,4, will save to/read from file: 4.16k)
- ESC exits.
- -t option traces Z80 execution.

# Possible further extensions
See Issues
