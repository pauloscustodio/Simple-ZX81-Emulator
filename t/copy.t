#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

unlink "zx81.txt";
make_p( "t/input/copy.bas", "t/input/copy.p" );
run_ok("zx81 t/input/copy.p");
check_text_file("t/expected/copy.txt", "zx81.txt");

unlink_testfiles();
done_testing();
