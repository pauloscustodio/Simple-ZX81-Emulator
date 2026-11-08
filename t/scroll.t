#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/scroll.bas", "t/input/scroll.p" );
run_ok("zx81 t/input/scroll.p");

unlink_testfiles();
done_testing();
