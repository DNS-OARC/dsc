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

my $dbh = get_dbh(AutoCommit => 1);
for my $name (@{&data_index_names($dbh)}) {
    print "dropping index $name\n";
    $dbh->do("DROP INDEX $name");
}
print "done.\n";


