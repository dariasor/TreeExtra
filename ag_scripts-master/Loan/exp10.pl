#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
     	while (<>) {
        	my $line = $_;
        	chomp($line);

		if ($line eq '?') {
			print "?\n";
        	} else {
			my $expval = 10 ** $line;
			print "$expval\n";
		}
	}
}

Main();
