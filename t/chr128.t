#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

make_p( "t/input/chr128.bas", "t/input/chr128.p" );
run_ok("zx81 t/input/chr128.p");

unlink_testfiles();
done_testing();
