#! /usr/bin/perl -w
use strict;
use warnings "all";

#my $threshold=0.602164;
my $threshold=0.5;

sub Main
{
     	while (<>) {
        	my $line = $_;
        	chomp($line);

		if (length $line) {
		    	my @features = split(' ', $line);
			if($features[0] < $threshold)
			{
				print "0\n";
			} else {
				print "$features[1]\n";
			}
		}
	}
}

Main();
