#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
	my $minval = $_[0];
	my $maxval = $_[1];
     	while (<STDIN>) {
        	my $line = $_;
        	chomp($line);

		if ($line eq '?') {
			print "?\n";
		}elsif ($line < $minval) {
             		print "$minval\n";         
		}elsif ($line > $maxval) {
             		print "$maxval\n";         
        	} else {
			print "$line\n";
		}
	}
}

Main($ARGV[0], $ARGV[1]);
