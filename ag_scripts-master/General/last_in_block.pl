#! /usr/bin/perl -w
use strict;
use warnings "all";

sub PairCmp
{
	my @apair = @$a;
	my @bpair = @$b;

	if ($apair[0] eq $bpair[0]) {
		$apair[1] <=> $bpair[1];
	} else {
		$apair[0] cmp $bpair[0];
	}
}

sub Main
{
	my @AoA = ();
	while (<>) {
		my @tmp = split;
        	push @AoA, [ @tmp ];
    	}

	my @sorted = sort PairCmp @AoA;

	my %hash = ();
	my $strN = scalar @AoA;
	for (my $strNo = 1; $strNo < $strN; $strNo++) {
		if($sorted[$strNo][0] eq $sorted[$strNo - 1][0]) {
			$hash{$sorted[$strNo - 1][0]}{$sorted[$strNo - 1][1]} = 0;
		} elsif (($strNo == 1) or ($sorted[$strNo - 1][0] ne $sorted[$strNo - 2][0])) {
			$hash{$sorted[$strNo - 1][0]}{$sorted[$strNo - 1][1]} = 0.5;
		} else {
			$hash{$sorted[$strNo - 1][0]}{$sorted[$strNo - 1][1]} = 1;
		}
	}
	if (($strN == 1) or ($sorted[$strN - 1][0] ne $sorted[$strN - 2][0])) {
		$hash{$sorted[$strN - 1][0]}{$sorted[$strN - 1][1]} = 0.5;
	} else {
		$hash{$sorted[$strN - 1][0]}{$sorted[$strN - 1][1]} = 1;
	}

	for (my $strNo = 0; $strNo < scalar @sorted; $strNo++) {
		print "$hash{$AoA[$strNo][0]}{$AoA[$strNo][1]}\n";
	}
}
Main();
