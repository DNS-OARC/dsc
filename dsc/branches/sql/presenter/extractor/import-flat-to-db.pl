#!/usr/bin/env perl
# This script is designed to be run from the source tree, without being
# installed, so it can run in parallel with an older installed version of
# extractor and presenter.

use warnings;
use strict;
use POSIX;

use FindBin;
use lib "$FindBin::Bin/../perllib";
use DSC::extractor;
use DSC::extractor::config;
use Data::Dumper;
use Getopt::Long;

my $DSCDIR = "/usr/local/dsc";
my $DATADIR = "$DSCDIR/data";
my $dbg = 0;
my $perfdbg = 0;

$ENV{TZ} = "UTC";

my %opts;
my $req_servers = undef;
my $req_nodes = undef;
my $mindate = 0;
my $today = strftime("%Y%m%d", gmtime((int(time / 86400)) * 86400));
my $maxdate = $today;
my $importcount = 0;

sub usage(@) {
    print STDERR @_;
    print STDERR "Usage: $0 [options]\n",
	"Options:\n",
	"--server=server1,server2...,serverN     limit to these servers\n",
	"--node=node1,node2...,nodeN             limit to these nodes\n",
	"--date=date                             limit to this day\n",
	"--date=[start]-[end]                    limit to this range of days\n",
	"Dates must be in the form YYYYMMDD, and are in UTC.\n",
	"Default date range is up to today (UTC), i.e. \"-$maxdate\".\n";
    exit 1;
}

sub descend_dir($) {
    my ($dir) = @_;
    chdir $dir;
    opendir DIR, "." || die "$0: reading $dir: $!\n";
    my @contents = readdir(DIR);
    closedir DIR;
    return @contents;
}

GetOptions(\%opts, "server=s", "node=s", "date=s", "datadir=s") or
    usage "$!\n";

map { $req_servers->{$_} = 1 } split(',', $opts{server}) if $opts{server};
map { $req_nodes->{$_} = 1 } split(',', $opts{node}) if $opts{node};
if (defined $opts{date}) {
    if ($opts{date} =~ /^(\d{8})?-(\d{8})?$/) {
	$mindate = $1 || $mindate;
	$maxdate = $2 || $maxdate;
	die "Date $mindate is greater than $maxdate.\n" if ($mindate>$maxdate);
    } elsif ($opts{date} =~ /^(\d{8})$/) {
	$mindate = $maxdate = $1;
    } else {
	usage;
    }
}
$DATADIR = $opts{datadir} || $DATADIR;

print "servers: ", $req_servers ? join ', ', keys %$req_servers : "(ALL)", "\n";
print "nodes: ", $req_nodes ? join ', ', keys %$req_nodes : "(ALL)", "\n";
print "dates: $mindate to $maxdate\n";
print "\n";

read_config("$DSCDIR/etc/dsc-extractor.cfg");

chdir $DATADIR || die "chdir $DATADIR: $!\n";

my $PROG=$0;
$PROG =~ s/^.*\///;
my $outfile = "$DATADIR/$PROG.stdout";

print "For further results, see $outfile\n";
open(STDOUT, ">$outfile") || die "$PROG: writing $outfile: $!\n";
open(STDERR, ">&1");

print strftime("%a %b %e %T %Z %Y", (gmtime)[0..5]), "\n";

my $dbh = get_dbh || die;
$dbh->{RaiseError} = 1;

my @servers = grep { $_ !~ /^\./ && !-l && -d } descend_dir($DATADIR);
for my $server (sort { $a cmp $b } @servers) {
    next if $req_servers && !$req_servers->{$server};
    my $server_id = get_server_id($dbh, $server);
    $dbh->commit;

    my @nodes = grep { $_ !~ /^\./ && !-l && -d } descend_dir($server);
    for my $node (sort { $a cmp $b } @nodes) {
	next if $req_nodes && !$req_nodes->{$node};
	my $node_id = get_node_id($dbh, $server_id, $node);
	$dbh->commit;
	print "server_id = $server_id, node_id = $node_id\n";
	my %maxday = ();

	my @dates = grep { /^\d{8}$/ && -d } descend_dir($node);
	for my $date (sort { $a cmp $b } @dates) {
	    next if ($date < $mindate || $date > $maxdate);
	    print "Importing $server/$node/$date\n";

	    $date =~ /^(\d{4})(\d{2})(\d{2})$/;
	    # $time is needed for accum data, which don't store time in file
	    my $time = mktime(0, 0, 0, $3, $2 - 1, $1 - 1900);

	    my @datasets = grep { $_ !~ /^\./ && -d } descend_dir($date);
	    for my $dataset (@datasets) {
		# print "## $server/$node/$date/$dataset\n";
		$dataset = "do_bit" if ($dataset eq "d0_bit"); # hack
		my $EX = $DSC::extractor::config::DATASETS{$dataset};
		
		die "no extractor for $dataset\n" unless defined($EX);

		my @outputs = grep { -f && /.dat$/ } descend_dir($dataset);
		for my $datafile (@outputs) {
		    # print "## $server/$node/$date/$dataset/$datafile\n";
		    my $output = $datafile;
		    $output =~ s/.dat$//;
		    $output = "do_bit" if ($output eq "d0_bit"); # hack
		    my $O = $EX->{outputs}{$output};

		    my $tabname = "dsc_$output";
		    if (!table_exists($dbh, $tabname)) {
			create_data_table($dbh, $tabname, $O->{dbkeys});
			$maxday{$output} = 0;

		    } elsif (!defined $maxday{$output}) {
			my ($maxtime) = $dbh->selectrow_array(
			    "SELECT MAX(start_time) FROM $tabname WHERE " .
			    "server_id = ? AND node_id = ?",
			    undef, $server_id, $node_id);
			$maxday{$output} = int(($maxtime||0) / 86400) * 86400;
		    }

		    if ($time < $maxday{$output}) {
			# full day was already imported
			next;
		    }

		    if ($time == $maxday{$output}) {
			# partial day was already imported; delete it.
			print "# replacing possibly incomplete day of data for $server/$node/$date/$dataset/$datafile\n";
			$dbh->do("DELETE FROM $tabname WHERE " .
			    "server_id = ? and node_id = ? AND start_time >= ?",
			    undef, $server_id, $node_id, $time);
		    }

		    my %data;
		    &{$O->{flat_reader}}(\%data, $datafile);
		    &{$O->{data_writer}}($dbh, \%data, $output,
			$server_id, $node_id, $time);
		    $importcount++;
		}
		chdir "..";
	    }
	    $dbh->commit;
	    chdir "..";
	}
	chdir "..";
    }
    chdir "..";
}
# end

END {
    print "Done: imported $importcount files.\n";
    print strftime("%a %b %e %T %Z %Y", (gmtime)[0..5]), "\n";
}


