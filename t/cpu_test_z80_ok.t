#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/cpu_test_z80_ok.bas", "t/input/cpu_test_z80_ok.p" );
capture_ok( "zx81 -d t/input/cpu_test_z80_ok.p",
    "t/expected/cpu_test_z80_ok.txt" );

unlink_testfiles();
done_testing();
