#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($)
{
	my $fname = $_[0];
     	open (my $in, "<", $fname) or die $!;
	my @lines = <$in>;
	
	my %hash = ();
	foreach my $line (@lines) {
		chomp($line);
		if ($line =~ /(.*) never/) {
			$hash{$1} = 1;
		}
	}	

    foreach my $line (@lines) {
		if (($line =~ /(.*):(.*)\./) and not ($line =~/\(class\)/)){
			if (!exists $hash{$1}) {
				print "$1\n";
			}
		}
	}
}

Main($ARGV[0]);

