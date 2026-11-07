#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/exit.bas", "t/input/exit.p" );
capture_ok( "zx81 t/input/exit.p", "t/expected/exit.txt" );

unlink_testfiles();
done_testing();
