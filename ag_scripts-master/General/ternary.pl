#! /usr/bin/perl -w
use strict;
use warnings "all";


sub Main
{
     	while (<>) {
        	my $line = $_;
        	chomp($line);

		if (length $line) {
		    	my @features = split('\t', $line);
			if($features[0] > 0)
			{
				print "$features[1]\n";
			} else {
				print "$features[2]\n";
			}
		}
	}
}

Main();
