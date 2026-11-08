#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p dump_p);

make_p( "t/input/dfile_collapsed.bas", "t/input/dfile_collapsed.p" );
run_ok("zx81 t/input/dfile_collapsed.p");

unlink_testfiles();
done_testing();
