#!/usr/bin/env perl
use warnings;
use strict;
use POSIX;

use FindBin;
use lib "$FindBin::Bin/../perllib";
use DSC::db;
use DSC::extractor;
use DSC::extractor::config;

my $DSCDIR = "/usr/local/dsc";

my $numdays = shift || die "usage: $0 numdays\n";
my $cutoff = time - ($numdays * 86400);

read_config("$DSCDIR/etc/dsc-extractor.cfg");

my $dbh = get_dbh;
$dbh->{AutoCommit} = 1;

$dbh->do("DELETE FROM loaded_files WHERE time <= $cutoff");

### You may customize this script here.

# $dbh->do("VACUUM ANALYZE"); # for PostgreSQL

