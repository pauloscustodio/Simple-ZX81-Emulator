#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p dump_p);

make_p( "t/input/load.bas", "t/input/load.p" );
run_ok("zx81 t/input/load.p");    # loads HELLO and runs it

unlink_testfiles();
done_testing();
