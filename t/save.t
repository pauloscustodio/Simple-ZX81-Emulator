#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p dump_p);

make_p( "t/input/save.bas", "t/input/save.p" );
unlink "save.p";
run_ok("zx81 t/input/save.p");    # creates save.p
dump_p( "save.p", "$test.txt" );
check_text_file( "t/expected/save.txt", "$test.txt" );

unlink "save.p";
unlink_testfiles();
done_testing();
