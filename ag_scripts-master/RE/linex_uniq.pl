#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($)
{
	my $fNo = $_[0] - 1;
	my %hash = ();
	while (<STDIN>) {
		my $line = $_;
		chomp($line);

		my @features = split(' ', $line);
		if (!exists $hash{$features[$fNo]}) {
			print "$line\n";
			$hash{$features[$fNo]} = 1;
		}
	}
}

Main($ARGV[0]);
