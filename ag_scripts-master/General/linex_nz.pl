#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
	my $classNo = $_[0];
     	while (<STDIN>) {
        	my $line = $_;
        	chomp($line);

		if (length $line) {
		    	my @features = split(' ', $line);
			if($features[$classNo - 1] > 0) {
				print "$line\n";
			}
		}
	}
}

Main($ARGV[0]);
