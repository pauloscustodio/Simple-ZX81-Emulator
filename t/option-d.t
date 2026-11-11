#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

unlink "zx81.txt";
make_p( "t/input/copy.bas", "t/input/copy.p" );
capture_ok( "zx81 -d t/input/copy.p", "t/expected/option-d.txt" );

unlink_testfiles();
done_testing();
