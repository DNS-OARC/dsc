#!/usr/bin/perl -wl

my $numdays = shift || die "usage: $0 numdays";
my $cutoff = time - ($numdays * 86400);

while (<>) {
	chomp;
	next unless (/\.xml/);
	next unless (m@/(\d+)\.@);
	my $t = $1;
	next if ($t > $cutoff);
	print "removing $_";
	unlink $_ or die "$_";
}
