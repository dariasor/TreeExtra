#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($$$)
{
	open (my $in1, "<", $_[0]) or die $!;
	my @raw_lines = <$in1>;
	open (my $in2, "<", $_[1]) or die $!;
	my @code_lines = <$in2>;
	open (my $in3, "<", $_[2]) or die $!;
	my @newcode_lines = <$in3>;
	my $newcode_line = $newcode_lines[0];
	chomp($newcode_line);
	my @new_codes = split('\t', $newcode_line);

	my $colN = 0;
	my @AoH = ();
	for (my $lNo = 0; $lNo < scalar @raw_lines; $lNo++) {
		my $raw_line = $raw_lines[$lNo];
		my $code_line = $code_lines[$lNo];
		chomp($raw_line);
		chomp($code_line);

		my @raw_vals = split('\t', $raw_line);
		my @code_vals = split('\t', $code_line);
		$colN = scalar @raw_vals;
		for (my $vNo = 0; $vNo < $colN; $vNo++) {
			if ($lNo == 0) {
				my $hash = {};
				push @AoH, $hash;
			}
			$AoH[$vNo]->{$raw_vals[$vNo]} = $code_vals[$vNo];
		}
	}

	while (<STDIN>) {
		my $line = $_;
		chomp($line);

		my @new_vals = split('\t', $line);
		for (my $vNo = 0; $vNo < $colN; $vNo++) {
			if (exists $AoH[$vNo]->{$new_vals[$vNo]}) {
				print "$AoH[$vNo]->{$new_vals[$vNo]}";
			} else {
				print "$new_codes[$vNo]";
			}
			if ($vNo != $colN - 1) {
				print "\t";
			}
		}
		print "\n";
	}
}


Main($ARGV[0], $ARGV[1], $ARGV[2]);
