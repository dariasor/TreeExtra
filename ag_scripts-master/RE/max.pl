#! /usr/bin/perl -w
use strict;
use warnings "all";
use List::Util qw[min max];

sub Main
{
     	while (<>) {
        	my $line = $_;
        	chomp($line);

		if (length $line) {
			my @features = split(' ', $line);
			my $f0 = $features[0];
			my $f1 = $features[1];
			if($f0 eq "?")
			{
				print "$f1\n";
			} 
			elsif ($f1 eq "?") 
			{ 
				print "$f0\n";
			} 
			else 
			{
				my $res = max($f0, $f1);
				print "$res\n";
			}
		}
	}
}

Main();
