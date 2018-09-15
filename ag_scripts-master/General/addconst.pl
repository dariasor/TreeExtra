#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($)
{
	my $const = $_[0];
     	while (<STDIN>) {
        	my $line = $_;
        	chomp($line);

		if ($line eq '?') {
			print "?\n";
        	} else {
			my $retval = $const + $line;
			print "$retval\n";
		}
	}
}

Main($ARGV[0]);
