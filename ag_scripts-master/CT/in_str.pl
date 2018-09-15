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
		$hash{$bline} = 1;
	}	

	while (<STDIN>) {
		my $aline = $_;
		chomp($aline);
		if (exists $hash{$aline}) {
			print "1\n";
		} else {
			print "0\n";
		}
    }
}

Main($ARGV[0]);

