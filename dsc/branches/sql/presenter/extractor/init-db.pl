#!/usr/bin/env perl

use warnings;
use strict;
use POSIX;

use FindBin;
use lib "$FindBin::Bin/../perllib";
use DSC::db;
use DSC::extractor::config;

my $DSCDIR = "/usr/local/dsc";

read_config("$DSCDIR/etc/dsc-extractor.cfg");

my $dbh = get_dbh;
$dbh->{AutoCommit} = 1;
generic_init_db($dbh);
specific_init_db($dbh);
print "done.\n";


