#!/usr/bin/env perl

use Modern::Perl;
use Path::Tiny;

@ARGV == 1 or die "Usage: $0 filename.bin\n";
my $bin_file = $ARGV[0];
my $bas_file = $bin_file =~ s/\.bin$/.bas/r;

my $bin_data  = path($bin_file)->slurp_raw;
my @bytes     = map { ord } split //, $bin_data;
my $hex_bytes = join '', map { sprintf "\\%02X", $_ } @bytes;

open( my $fh, ">", $bas_file ) or die "Cannot open $bas_file: $!";
say $fh "0 REM $hex_bytes";
close($fh);
