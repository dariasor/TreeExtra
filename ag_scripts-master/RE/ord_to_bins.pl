#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
    while (<>) {
        my $line = $_;
        chomp($line);

		if (length $line) {
			if($line eq "?"){
				print "?\t?\n";
			} else {
                my $f1 = 0;
                my $f2 = 0;
                if ($line == 1) {
                    $f1 = 1;
                } elsif ($line == 2) {
                    $f2 = 1;
                }
				print "$f1\t$f2\n";
			}
		}
	}
}

Main();
