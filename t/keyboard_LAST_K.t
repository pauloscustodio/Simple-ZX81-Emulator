#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p dump_p);

run_ok(
    "perl tools/filepp-1.8.0/filepp -UROM -o $test.bas t/input/keyboard.bas");
make_p( "$test.bas", "$test.p" );
run_ok("zx81 -s $test.p");

unlink_testfiles();
done_testing();
