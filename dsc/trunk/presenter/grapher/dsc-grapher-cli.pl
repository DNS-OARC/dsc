#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long;
use DSC::grapher;

my %args;
my $result = GetOptions (\%args,
			"server=s",
			"node=s",
			"window=s",
			"binsize=i",
			"plot=s",
			"end=i",
			"yaxis=s",
			"key=s",
			);

DSC::grapher::prepare();
DSC::grapher::run(undef, \%args);
