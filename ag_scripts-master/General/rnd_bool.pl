#! /usr/bin/perl -w
use strict;
use warnings "all";


sub Main($$)
{
    my $prob = $_[0];
    my $lineN = $_[1];
    for(my $lNo = 0; $lNo < $lineN; $lNo++) {
        if(rand() < $prob) {
            print "1\n";
        } else {
            print "0\n";
        }
    }
}

Main($ARGV[0], $ARGV[1]);
