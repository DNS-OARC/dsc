#!/usr/bin/env perl

use strict;
use warnings;

my $numdays = shift || die "usage: $0 numdays";
my $cutoff = time - ($numdays * 86400);

while (<>) {
	chomp;
	next unless (/\.xml/);
	next unless (m@/\d\d\d\d\d\d\d\d/[^/]+/(\d+)\.@);
	my $t = $1;
	next if ($t > $cutoff);
	#print "removing $_\n";
	unlink $_ or die "$_";
}
