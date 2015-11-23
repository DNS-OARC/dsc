#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long;
use DSC::grapher;

our $ploticus_debug=1;

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

my $grapher = DSC::grapher->new();
$grapher->cmdline(\%args);
$grapher->run();
