#! /usr/bin/perl -w
use strict;
use warnings "all";


sub Main($$)
{
    my $valN = $_[0];
    my $lineN = $_[1];
    for(my $lNo = 0; $lNo < $lineN; $lNo++) {
        my $val = int(rand()*$valN);
        print "$val\n";
    }
}

Main($ARGV[0], $ARGV[1]);
