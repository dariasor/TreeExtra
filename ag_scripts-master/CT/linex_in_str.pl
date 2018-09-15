#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main($$$)
{
	my $fname = $_[0];
	my $bkeyno = $_[1] - 1;
	my $akeyno = $_[2] - 1;
     	open (my $in, "<", $fname) or die $!;
	my @blines = <$in>;
	
	my %hash = ();
	foreach my $bline (@blines) {
        chomp($bline);
		my @bfeatures = split('\t', $bline);
		my $bkey = $bfeatures[$bkeyno];
		$hash{$bkey} = 1;
	}	

	while (<STDIN>) {
		my $aline = $_;
		chomp($aline);
		my @afeatures = split('\t', $aline);
		my $akey = $afeatures[$akeyno];
		if (exists $hash{$akey}) {
			print "$aline\n";
		} 		
    }
}

Main($ARGV[0],$ARGV[1],$ARGV[2]);

