#! /usr/bin/perl -w
use strict;
use warnings "all";

sub BigIntCmp
{
	if (length $a == length $b) {
		$a cmp $b;
	} else {
		length $a <=> length $b;
	}
}

sub Main
{
	my @strings = <STDIN>;
	foreach my $str (@strings) {
		chomp $str;
	}
	my @stringsSrt = sort BigIntCmp @strings;

	my %hash = ();
	$hash{$stringsSrt[0]} = 0;
	for (my $strNo=1; $strNo < scalar @stringsSrt; $strNo++) {
		if ($stringsSrt[$strNo] ne $stringsSrt[$strNo - 1]) {
			$hash{$stringsSrt[$strNo]} = $strNo;
		}
	}

	foreach my $str (@strings) {
		print "$hash{$str}\n";
	}
}

Main();