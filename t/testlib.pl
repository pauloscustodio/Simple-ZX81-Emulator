#!/usr/bin/env perl

#------------------------------------------------------------------------------
# Simple-ZX81-Emulator
# Based on https://leiradel.github.io/2011/06/11/How-to-Emulate.html
# Modifications Copyright (C) Paulo Custodio, 2026
# License: GPL
#------------------------------------------------------------------------------

use Modern::Perl;
use Test::More;
use Test::HexDifferences;
use Config;
use Path::Tiny;
use Text::Diff;
use File::Copy;
use File::Spec;
use Text::ParseWords qw(shellwords);
use ZX81::Tape       qw(dump_p);

$ENV{PATH} = join( $Config{path_sep}, ".", $ENV{PATH} );

use vars '$test', '$null';
$test = "test_" . ( $0 =~ s/\W/_/gr );
$null = ( $^O eq 'MSWin32' ) ? 'nul' : '/dev/null';

unlink_testfiles();

#------------------------------------------------------------------------------
sub check_text_file {
    my ( $exp_file, $got_file ) = @_;
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    ( my $exp_text = path($exp_file)->slurp ) =~ s/\r\n/\n/g;
    ( my $got_text = path($got_file)->slurp ) =~ s/\r\n/\n/g;

    # remove version
    $got_text =~ s/Version: .*//;
    path($got_file)->spew_raw($got_text);

    note "text diff expected ($exp_file) got ($got_file)";
    my $diff = diff( \$exp_text, \$got_text, { STYLE => 'Unified' } );
    if ( $diff ne "" ) {
        diag $diff;
        if ( $ENV{DEVELOPER} ) {
            system( "start \"\" WinMergeU.exe " . "$exp_file $got_file" );
        }
    }
    ok $diff eq "", "text files are equal";

    ( Test::More->builder->is_passing ) or die;
}

#------------------------------------------------------------------------------
sub compare_p {
    my ( $exp_p, $got_p ) = @_;

    my $exp_bytes = path($exp_p)->slurp_raw;
    my $got_bytes = path($got_p)->slurp_raw;

    eq_or_dump_diff(
        $exp_bytes, $got_bytes,
        { address => 0x4009 },
        "diff $exp_p $got_p"
    );
    if ( $exp_bytes ne $got_bytes ) {
        if ( $ENV{DEVELOPER} ) {
            dump_p( $exp_p, "$exp_p.txt" );
            dump_p( $got_p, "$got_p.txt" );
            system "start \"\" WinMergeU.exe $exp_p.txt $got_p.txt";
        }
    }

    if ( Test::More->builder->is_passing ) {
        unlink "$exp_p.txt", "$got_p.txt";
    }
}

#------------------------------------------------------------------------------
sub run_ok {
    my ($cmd) = @_;
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    $cmd = normalize_command($cmd);
    ok 1,                 "Running: $cmd";
    ok 0 == system($cmd), $cmd;

    ( Test::More->builder->is_passing ) or die;
}

#------------------------------------------------------------------------------
sub run_nok {
    my ($cmd) = @_;
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    $cmd = normalize_command($cmd);
    ok 1,                 "Running: $cmd";
    ok 0 != system($cmd), $cmd;

    ( Test::More->builder->is_passing ) or die;
}

#------------------------------------------------------------------------------
sub capture_ok {
    my ( $cmd, $exp_out ) = @_;
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    run_ok( $cmd . " > ${test}.stdout" );
    check_text_file( $exp_out, "${test}.stdout" );

    ( Test::More->builder->is_passing ) or die;
}

#------------------------------------------------------------------------------
sub capture_nok {
    my ( $cmd, $exp_err ) = @_;
    local $Test::Builder::Level = $Test::Builder::Level + 1;

    run_nok( $cmd . " 2> ${test}.stderr" );
    check_text_file( $exp_err, "${test}.stderr" );

    ( Test::More->builder->is_passing ) or die;
}

#------------------------------------------------------------------------------
sub unlink_testfiles {
    my (@additional) = @_;
    unlink( <${test}*>, @additional )
        if Test::More->builder->is_passing;
}

#------------------------------------------------------------------------------
# need to normalize slashes in the command only, not in the arguments,
# so that Strawberry Perl is able to start build/Debug/z88dk-z80asm.exe
sub normalize_command {
    my ($cmd) = @_;

    # Split command into tokens safely
    my @tokens = shellwords($cmd);

    # Normalize only the executable path
    if ( $^O eq 'MSWin32' ) {
        my @parts = split( '/', $tokens[0] );
        $tokens[0] = File::Spec->catfile(@parts);
    }

    # Reassemble command
    my $fixed_cmd = join( ' ', @tokens );
    return $fixed_cmd;
}

1;
