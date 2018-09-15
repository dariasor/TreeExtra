#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($$)
{
	my $classNo = $_[0];
	my $value = $_[1];
     	while (<STDIN>) {
        	my $line = $_;
        	chomp($line);

		if (length $line) {
		    my @features = split('\t', $line);
			if($features[$classNo - 1] == $value) {
				print "$line\n";
			}
		}
	}
}

Main($ARGV[0],$ARGV[1]);
