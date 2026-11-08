#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/slow.bas", "t/input/slow.p" );
run_ok("zx81 -s t/input/slow.p");

unlink_testfiles();
done_testing();
