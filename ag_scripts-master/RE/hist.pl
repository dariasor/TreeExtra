#! /usr/bin/perl -w
use strict;
use warnings "all";
#use Math::Round;

sub Main($)
{
	my %hash = ();
	my $w = $_[0];
	while (<STDIN>) {
		my $value = $_;
		chomp($value);
		my $key = round($value/$w) + 0.0; #the last part gets rid of -0.0 values
		$hash{$key} += 1;
	}
	my @hashkeys = sort {$a <=> $b} keys %hash;
	foreach my $key (@hashkeys) {
		my $histval = $key * $w;
		print "$histval\t$hash{$key}\n";
	} 
}

Main($ARGV[0]);
