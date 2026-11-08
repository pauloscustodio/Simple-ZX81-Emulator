#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p dump_p);

# only run in DEVELOPER mode, because it requires the user to quit the emulator
if ( $ENV{DEVELOPER} ) {
make_p( "t/input/load.bas", "t/input/load.p" );
run_ok("zx81 t/input/load.p");  # loads HELLO and runs it
}

unlink_testfiles();
done_testing();
