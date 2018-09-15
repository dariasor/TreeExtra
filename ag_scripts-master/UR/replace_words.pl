#! /usr/bin/perl -w
use strict;
use warnings "all";
use Path::Tiny qw(path);

sub Main($)
{
	my $filename = $_[0];
	my $file = path($filename);
	my $text = $file->slurp_utf8;
	while (<STDIN>) {
		my $line = $_;
		chomp($line);
		my @words = split('\t', $line);
		$text =~ s/$words[0]/$words[1]/g;
	}
	$file->spew_utf8($text);
}


Main($ARGV[0]);
