#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
     	while (<>) {
        	my $line = $_;
        	chomp($line);

		if (length $line) {
		    	my @features = split(' ', $line);
			if(($features[0] eq "?") or ($features[1] eq "?"))
			{
				print "?\n";
			} else {
				my $subtr = $features[0]-$features[1];
				print "$subtr\n";
			}
		}
	}
}

Main();
