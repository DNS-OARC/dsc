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
my $start = time;
my $n = 0;
for my $name (@{&data_table_names($dbh)}) {
    print "creating indexes for $name\n";
    create_data_indexes($dbh, $name);
    my $now = time;
    print "created indexes for $name in ", $now - $start, " seconds \n";
    $start = $now;
    $n++;
}
print "Done creating indexes for $n tables.\n";


