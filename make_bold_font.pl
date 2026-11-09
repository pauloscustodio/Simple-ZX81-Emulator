#!/usr/bin/env perl

# make a bold version of the zx81 font

use Modern::Perl;
use Path::Tiny;
use Data::Dump 'dump';

@ARGV == 1 or die "Usage: $0 rom\n";

# read zx81 rom
my @rom = map { ord } split //, path( $ARGV[0] )->slurp_raw;

# collect font data
my @font = @rom[ 0x1e00 .. 0x1fff ];

# convert to binary
for (@font) { $_ = sprintf( "0b%08b", $_ ); }

# make bold
for (@font) { 1 while s/0100/0110/; }

# convert back to numbers
for (@font) { $_ = eval($_); $@ and die $@; }

# outpus asm code
for (@font) { printf "        defb    0b%08b\n", $_; }
