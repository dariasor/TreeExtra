#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
     	while (<>) {
        	my $line = $_;
        	chomp($line);

        	if ($line =~ /.*00000.*/) {
             		print "1\n";         
        	} else {
			print "0\n";
		}
	}
}

Main();
