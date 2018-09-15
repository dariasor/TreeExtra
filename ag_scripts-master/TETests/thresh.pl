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
		}elsif ($line <= 0) {
             		print "0\n";         
        	} else {
			print "1\n";
		}
	}
}

Main();
