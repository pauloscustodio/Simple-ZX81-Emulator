#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/font.bas", "t/input/font.p" );
run_ok("zx81 t/input/font.p");

unlink_testfiles();
done_testing();
