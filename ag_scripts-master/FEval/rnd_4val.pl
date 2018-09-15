#! /usr/bin/perl -w
use strict;
use warnings "all";


sub Main($$$$)
{
    my $prob1 = $_[0];
    my $prob2 = $_[1] + $prob1;
    my $prob3 = $_[2] + $prob2;
    my $lineN = $_[3];
    for(my $lNo = 0; $lNo < $lineN; $lNo++) {
        my $rnd = rand();
        if($rnd < $prob1) {
            print "1\n";
        } elsif($rnd < $prob2) {
            print "2\n";
        } elsif($rnd < $prob3) {
            print "3\n";
        } else {
            print "0\n";
        }
    }
}

Main($ARGV[0], $ARGV[1], $ARGV[2], $ARGV[3]);
