#! /usr/bin/perl -w
use strict;
use warnings "all";

sub Main
{
	while (<>) {
          	my $line = $_;
          	chomp($line);
          	my @features = split(' ', $line);
          	foreach my $feature (@features) {
               		if ($feature =~ /.*:(.*)/) {
                    		print "\t$1";         
               		} else {
				print $feature;
			}			
     		}
		print "\n";
	}
}


Main();
