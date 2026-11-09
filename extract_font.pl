#!/usr/bin/env perl

# extract the zx81 font from the rom

use Modern::Perl;
use Path::Tiny;
use Data::Dump 'dump';

@ARGV == 1 or die "Usage: $0 rom\n";

# read zx81 rom
my @rom = map { ord } split //, path( $ARGV[0] )->slurp_raw;

# collect font data
my @font = @rom[ 0x1e00 .. 0x1fff ];

# outpus asm code
for (@font) { printf "        defb    0b%08b\n", $_; }
