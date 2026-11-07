#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/exit.bas", "t/input/exit.p" );
run_ok("zx81 -t t/input/exit.p | head -n 100 > $test.txt");
check_text_file( "t/expected/trace.txt", "$test.txt" );

unlink_testfiles();
done_testing();
