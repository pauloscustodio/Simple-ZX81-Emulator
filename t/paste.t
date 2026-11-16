#!/usr/bin/env perl

BEGIN { use lib 't'; require 'testlib.pl'; }

use Modern::Perl;
use ZX81::Tape qw(make_p);

my $NUM_TERMS = 20;    # stress test with 220

# load program via .p
my $sum = make_bas_file( "$test.1.bas", "#AUTOSTART=1" );
make_p( "$test.1.bas", "$test.1.p" );
ok -s "$test.1.p" < 0x8000 - 0x4009, "$test.1.p size " . ( -s "$test.1.p" );

unlink "zx81.txt";
run_ok("zx81 $test.1.p");
my $computed_sum = 0 + ( path("zx81.txt")->slurp );
is $computed_sum, $sum, "computed sum $sum";

# load program via paste
$sum = make_bas_file( "$test.2.bas", "RUN" );
unlink "zx81.txt";
run_ok("zx81 $test.2.bas");
$computed_sum = 0 + ( path("zx81.txt")->slurp );
is $computed_sum, $sum, "computed sum $sum";

unlink "zx81.txt";
unlink_testfiles();
done_testing();

# build BASIC program to compute a number
sub make_bas_file {
    my ( $filename, $last_line ) = @_;

    open( my $fh, ">", $filename ) or die "Open file $filename failed: $!";
    my $sum = 0;
    say $fh "1 LET S=0";
    for my $n ( 1 .. $NUM_TERMS ) {
        say $fh $n * 2,     " LET S=S*257+$n";
        say $fh $n * 2 + 1, " LET S=S-65521*INT(S/65521)";
        $sum = ( $sum * 257 + $n ) % 65521;
    }
    say $fh "9992 PRINT S";
    say $fh "9993 LPRINT S";
    say $fh "9994 PAUSE 50";
    say $fh "9995 RAND USR 8191";
    say $fh $last_line;
    close $fh;
    return $sum;
}
