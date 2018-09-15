#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($)
{
     my $fname = $_[0];
     open (my $in, "<", $fname) or die $!;
     my @blines = <$in>;

    my %hash = ();
    foreach my $bline (@blines) {
        chomp($bline);
		my @bfeatures = split('\t', $bline);
        $hash{$bfeatures[0]} = 1;
    }

	while (<STDIN>) {
		my $aline = $_;
		chomp($aline);
		my @afeatures = split('\t', $aline);
		if (@afeatures >= 2 and exists $hash{$afeatures[0]} and exists $hash{$afeatures[1]}) {
			print "$aline\n";
		}
	}
}

Main($ARGV[0]);
