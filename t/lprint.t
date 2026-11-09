#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

unlink "zx81.txt";
make_p( "t/input/lprint.bas", "t/input/lprint.p" );
run_ok("zx81 t/input/lprint.p");
check_text_file("t/expected/lprint.txt", "zx81.txt");

unlink_testfiles();
done_testing();
