#!/usr/bin/env perl

use warnings;
use strict;
use POSIX;

use DSC::db;
use DSC::extractor;
use DSC::extractor qw($SKIPPED_KEY $SKIPPED_SUM_KEY);
use DSC::extractor::config;
use Data::Dumper;
use Time::HiRes; # XXX
use Getopt::Std;

my %opts;
getopts('dp', \%opts);
my $DSCDIR = "/usr/local/dsc";
my $DATADIR = "$DSCDIR/data";

read_config("$DSCDIR/etc/dsc-extractor.cfg");

chdir $DATADIR || die "chdir $DATADIR: $!\n";

my $PROG=$0;
$PROG =~ s/^.*\///;

unless ($opts{d}) {
open(STDOUT, ">$PROG.stdout") || die "$PROG: writing $PROG.stdout: $!\n";
open(STDERR, ">&1");
}

print strftime("%a %b %e %T %Z %Y", (gmtime)[0..5]), "\n";

opendir DATADIR, "." || die "$0: reading $DATADIR: $!\n";
my @servers = grep { $_ !~ /^\./ && !-l && -d } readdir(DATADIR);
closedir DATADIR;
for my $server (@servers) {

    # get server id, or insert one if it does not exist
    my $dbh = get_dbh || die;
    $dbh->{RaiseError} = 1;
    my $server_id = get_server_id($dbh, $server);
    $dbh->commit;
    $dbh->disconnect;
    $dbh = undef;

    chdir "$DATADIR/$server";
    opendir SERVER, "." || die "$0: reading $DATADIR/$server: $!\n";
    my @nodes = grep { $_ !~ /^\./ && !-l && -d } readdir(SERVER);
    closedir SERVER;
    for my $node (@nodes) {
	chdir "$DATADIR/$server/$node";
	print "$server/$node:\n";
	my $pid = fork;
	defined $pid || die "fork: $!\n";
	if ($pid == 0) {
	    &refile_and_grok_node($server, $server_id, $node);
	    exit;
	}
    }
}

while (wait > 0) { }
print strftime("%a %b %e %T %Z %Y", (gmtime)[0..5]), "\n";
# end

my $mark_start;
sub mark {
	return unless $opts{p};
	my $msg = shift;
	unless ($msg) {
		$mark_start = Time::HiRes::gettimeofday;
		return;
	}
	printf STDERR "MARK %7.2f :: $msg\n",
		Time::HiRes::gettimeofday - $mark_start;
}

sub refile_and_grok_node($$$) {
    my ($server, $server_id, $node) = @_;
print STDERR "refile_and_grok_node($server,$server_id,$node)\n" if $opts{d};
    my $node_id = 0;
    my $pidf_is_mine = 0;
    my ($sth, @row);
    my $dbh = get_dbh || die;
    $dbh->{RaiseError} = 1;

    $PROG = "$PROG-$server-$node";

    my $PIDF="/var/tmp/$PROG.pid";
    if (open PIDF, "<$PIDF") {
	if (kill 0, <PIDF>) {
	    exit 0;
	}
	close PIDF;
    }

    END {
	if ($pidf_is_mine) { unlink $PIDF; }
    }

    $pidf_is_mine = 1;
    open PIDF, ">$PIDF" || die "$PROG: writing $PIDF: $!\n";
    print PIDF "$$\n";
    close PIDF;

unless ($opts{d}) {
    open(STDOUT, ">$PROG.stdout") || die "$PROG: writing $PROG.stdout: $!\n";
    open(STDERR, ">$PROG.stderr") || die "$PROG: writing $PROG.stderr: $!\n";
}

    print strftime("%a %b %e %T %Z %Y", (gmtime)[0..5]), "\n";

    # get node id, or insert one if it does not exist
    $node_id = get_node_id($dbh, $server_id, $node);
    $dbh->commit;

    print "server_id = $server_id, node_id = $node_id\n";

    my $DIR;
    foreach my $tdir (<incoming/*>, '.') {
	$DIR=$tdir;
	last if -d $DIR;
    }

    opendir DIR, $DIR || die "$0: reading $DIR: $!\n";
    my @xmls = map { $_->[0] }		# Take the [0] of each pair...
	sort { $a->[1] <=> $b->[1] }	# of a list of pairs sorted by [1]...
	map { [$_, /(\d+)/ ] }		# where [0] is name and [1] is time...
	grep { /\.xml$/ } readdir(DIR);	# for each *.xml filename in DIR.
    closedir DIR;

    my $n = 0;
    my $ts1 = Time::HiRes::gettimeofday;
    for my $h (@xmls) {
	if (!($h =~ /(\d+)\.([^.]*)\.xml$/)) { next; }
	if (++$n > 100) { last };
	my $secs = $1 - 60;
	my $type = $2;

	# hack
	if ($type eq "d0_bit") {
	    my $toname = $h;
	    $toname = s/d0_bit/do_bit/;
	    rename "$DIR/$h", "$DIR/$toname" || die "$0: rename $h: $!\n";
	    $type='do_bit';
	    $h = $toname;
	}

	my $xml_result = eval { extract_xml($dbh, "$DIR/$h", $server_id, $node_id) };
	if (!defined $xml_result) {
	    # other error
	    -d "errors" || mkdir "errors";
	    print STDERR "error processing $server/$node/$DIR/$h\n";
	    print STDERR "$@\n" if $@;
	    rename "$DIR/$h", "$DIR/errors/$h";
	    $dbh->rollback;
	} elsif ($xml_result == 0) {
	    # error while reading file
	    print "problem reading $type data file for $server/$node\n";
	    $dbh->rollback;
	    last;
	} else {
	    # success
	    $dbh->commit;
	    unlink "$DIR/$h" || die "$0: unlink $h: $!\n";
	}
    } continue {
	my $ts2 = Time::HiRes::gettimeofday;
	printf STDERR "$server/$node/$h took %.1f seconds\n", $ts2-$ts1;
	$ts1 = $ts2;
    }

    print strftime("%a %b %e %T %Z %Y", (gmtime)[0..5]), "\n";

    if (-s "$PROG.stderr") {
	$ENV{SUBJECT} = $0;
	$ENV{TEXT} = "$0 for $server/$node:";
	$ENV{PROG} = $PROG;
	system("{ echo \"\$TEXT\"; echo; " .
	        "/usr/bin/uniq \${PROG}.stderr | head -20; } " .
		"| /usr/bin/Mail -s \"Cron <\${USER}@`hostname -s`> \$SUBJECT\" \$USER");
    }
}

sub extract_xml($$$) {
    mark(undef);
    my ($dbh, $xmlfile, $server_id, $node_id) = @_;
    die "cant divine dataset" unless ($xmlfile =~ /(\d+)\.(\w+)\.xml$/);
    my $file_time = $1;
    my $dataset = $2;
    print STDERR "dataset is $dataset\n" if ($opts{d});

    my $EX = $DSC::extractor::config::DATASETS{$dataset};
    print STDERR 'EX=', Dumper($EX) if ($opts{d});
    die "no extractor for $dataset\n" unless defined($EX);

    my $sth = $dbh->prepare("SELECT 1 " . from_dummy($dbh) .
	" WHERE EXISTS (SELECT 1 " .
	"FROM loaded_files WHERE time = ? AND " .
	"dataset = ? AND " .
	"server_id = ? AND node_id = ?)");
    $sth->execute($file_time, $dataset, $server_id, $node_id);
    if ($sth->fetchrow_array) {
	print STDERR "skipping duplicate $xmlfile\n";
	return 1; # do nothing, successfully
    }

    my $start_time;
    my $grokked;
    if ($EX->{ndim} == 1) {
	    ($start_time, $grokked) = grok_1d_xml($xmlfile, $EX->{type1});
    } elsif ($EX->{ndim} == 2) {
	    ($start_time, $grokked) = grok_2d_xml($xmlfile, $EX->{type1}, $EX->{type2});
    } else {
	    die "unsupported ndim $EX->{ndim}\n";
    }
    mark("grokked $xmlfile");
    # round start time down to start of the minute
    $start_time = $start_time - $start_time % 60;
    my $yymmdd = &yymmdd($start_time);

    print STDERR 'grokked=', Dumper($grokked) if ($opts{d});

    foreach my $output (keys %{$EX->{outputs}}) {
	print STDERR "output=$output\n" if ($opts{d});
	my %db;
	my $O =  $EX->{outputs}{$output};

	# transform the input into something for this particular
	# output format
	#
	my $grok_copy;
	if (defined($O->{data_munger})) {
		$grok_copy = &{$O->{data_munger}}($grokked, $O);
	} else {
		$grok_copy = $grokked;
	}
	print STDERR 'POST MUNGE grok_copy=', Dumper($grok_copy) if ($opts{d});
	mark("munged $output");

	my $tabname = "dsc_$output";
	my $withtime = scalar (grep { /start_time/ } @{$O->{dbkeys}});
	my $bucket_time = $withtime ? $start_time :
	    $start_time - $start_time % 86400; # round down to start of day

	# If this is the last data of the UTC day (i.e., time is 23:59:00), we
	# will move the whole day's data from the "new" to the "old" table.
	my $archivable = ($start_time % 86400) == (23*3600 + 59*60);

	if (!table_exists($dbh, "${tabname}_new")) {
	    eval {
		create_data_table($dbh, $tabname, $O->{dbkeys});
		create_data_indexes($dbh, $tabname);
	        mark("table and index created");
	    };
	    if ($@) {
		my $SQLSTATE_UNIQUE_VIOLATION = "23505";
		if ($dbh->err && $dbh->state eq $SQLSTATE_UNIQUE_VIOLATION) {
		    # This could happen if a sibling process beat us to it.
		    # (We could avoid this with locks, but that would be more
		    # expensive, and this case is very rare, only possible
		    # the first time a new dataset it used.)
		    print "table $tabname already exists.\n";
		    $dbh->rollback;
		} else {
		    print STDERR "error=", $dbh->err, " state=", $dbh->state,
			" errstr=", $dbh->errstr, "\n";
		    die $@;
		}
	    }

	} elsif (!$withtime) {
	    # read and delete the existing data
	    return 0 if (0 > DSC::db::read_data($dbh, \%db, $output,
		[], [$node_id], $bucket_time, undef, 1, $O->{dbkeys}));
	    mark("read_data done");
	    my $where_clause = "WHERE start_time = $bucket_time AND " .
		"server_id = $server_id AND node_id = $node_id";
	    # Normally, the existing data is in the _new table.  But we must
	    # also delete from _old, since it may contain data inserted by
	    # by import-flat-to-db.  (When the extra delete is not needed, the
	    # index should make it very fast.)
	    $dbh->do("DELETE FROM ${tabname}_new $where_clause");
	    $dbh->do("DELETE FROM ${tabname}_old $where_clause");
	    mark("DELETEs done");
	}

	my $insert_suffix;
	if ($archivable && !$withtime) {
	    # write full 1-day dataset directly to _old table
	    $insert_suffix = 'old';
	} else {
	    # quickly write 1-minute or partial day dataset to _new table
	    $insert_suffix = 'new';
	}

	# merge/combine
	#
	mark("merge starting");
	&{$O->{data_merger}}($start_time, \%db, $grok_copy);
	print STDERR 'POST MERGE db=', Dumper(\%db) if ($opts{d});
	mark("merged $output");

	# trim
	#
	if (defined($O->{data_trimer}) && ((59*60) == ($start_time % 3600))) {
	    my $ntrim = &{$O->{data_trimer}}(\%db, $O);
	    print "trimmed $ntrim records from $output\n" if ($ntrim);
	    mark("trimmed $output");
	}

	# write out the new data
	#
	&{$O->{data_writer}}($dbh, \%db, "${output}_${insert_suffix}",
	    $server_id, $node_id, $bucket_time);
	mark("stored $output");

	if ($archivable) {
	    # Move old data from _new table to _old table.  We do this even
	    # on non-withtime tables in case a previous archival was missed).
	    my $where_clause = "WHERE start_time <= $bucket_time " .
		"AND server_id = $server_id AND node_id = $node_id";
	    $dbh->do("INSERT INTO ${tabname}_old " .
		"SELECT * FROM ${tabname}_new $where_clause");
	    $dbh->do("DELETE FROM ${tabname}_new $where_clause");
	    mark("archived $output");
	}
    }

    # Remember that we've processed this file.
    $sth = $dbh->prepare("INSERT INTO loaded_files " .
	"(time, dataset, server_id, node_id) VALUES (?, ?, ?, ?)");
    $sth->execute($file_time, $dataset, $server_id, $node_id);

    return 1; # success
}

#
# basically a wrapper for elsify_unwanted_keys();

sub munge_elsify {
	my $input = shift;	# XML tree from input file
	my $O = shift;		# extractor->output structure
	# copy the input data
	my $copy;
	eval Data::Dumper->Dump([$input], ['copy']);
	if ($O->{keys}) {
		# A 1D dataset
		print STDERR "elsifiying 1D dataset\n" if ($opts{d});
		elsify_unwanted_keys($copy, $O->{keys});
	} elsif ($O->{keys2}) {
		# A 2D dataset
		print STDERR "elsifiying 2D dataset\n" if ($opts{d});
		foreach my $k1 (keys %$copy) {
			&elsify_unwanted_keys(\%{$copy->{$k1}}, $O->{keys2});
		}
	} else {
		die "not sure what to do";
	}
	$copy;
}

#
# convert a "2D" accum-type dataset to a trace-type
# ie, sum all input->{*}{k2} into output->{k2}
#
sub accum2d_to_trace {
	my $input = shift;	# XML tree from input file
	my $O = shift;		# extractor->output structure
	my $trace;
	foreach my $k1 (keys %{$input}) {
		$trace->{$k1} = 0;
		foreach my $k2 (keys %{$input->{$k1}}) {
			next if ($k2 eq $SKIPPED_KEY);
			$trace->{$k1} += $input->{$k1}{$k2};
		}
	}
	$trace;
}

#
# convert a "1D" accum-type dataset to a "count-type"
# ie, count all input->{k1}
#
sub accum1d_to_count {
	my $input = shift;	# XML tree from input file
	my $O = shift;		# extractor->output structure
	my $count = 0;
	foreach my $k1 (keys %{$input}) {
		if ($k1 eq $SKIPPED_SUM_KEY) {
			next;
		} elsif ($k1 eq $SKIPPED_KEY) {
			$count += $input->{$k1};
		} else {
			$count += 1;
		}
	}
	$count;
}

#
# convert a "2D" accum-type dataset to a "count-type"
# ie, count all input->{*}{k2} into output->{k2}
#
sub accum2d_to_count {
	my $input = shift;	# XML tree from input file
	my $O = shift;		# extractor->output structure
	my $count;
	foreach my $k1 (keys %{$input}) {
		$count->{$k1} = 0;
		foreach my $k2 (keys %{$input->{$k1}}) {
			if ($k2 eq $SKIPPED_SUM_KEY) {
				next;
			} elsif ($k2 eq $SKIPPED_KEY) {
				$count->{$k1} += $input->{$k1}{$k2};
			} else {
				$count->{$k1} += 1;
			}
		}
	}
	$count;
}


sub merge_trace {
	my $start_time = shift;
	my $old = shift;
	my $new = shift;
	$old->{$start_time} = $new;
}

sub merge_accum1d {
	my $start_time = shift;
	my $old = shift;
	my $new = shift;
	foreach my $k1 (keys %{$new}) {
		$old->{$k1} += $new->{$k1};
	}
}

sub merge_accum2d {
	my $start_time = shift;
	my $old = shift;
	my $new = shift;
	foreach my $k1 (keys %{$new}) {
		foreach my $k2 (keys %{$new->{$k1}}) {
			$old->{$k1}{$k2} += $new->{$k1}{$k2};
		}
	}
}

sub swap_dimensions {
	my $input = shift;
	my $O = shift;
	my $output;
	foreach my $k1 (keys %{$input}) {
		foreach my $k2 (keys %{$input->{$k1}}) {
			$output->{$k2}{$k1} += $input->{$k1}{$k2};
		}
	}
	$output;
}

#
# this function is pretty ugly because we've chosen to swap the order of they
# keys before writing the datafile.  It would be simpler if the 1st and 2nd
# dimensions were swapped, but that would mean we need to swap the keys
# when plotting, not to mention reformatting all the archived data files
#
sub trim_accum2d {
	my $data = shift;
	my $O = shift;
	my $ndel = 0;
	foreach my $k2 (@{$O->{keys} || $O->{keys2}}) {
		my $n = 0;
		foreach my $k1 (sort {($data->{$b}{$k2} || 0) <=> ($data->{$a}{$k2} || 0)} keys %$data) {
			next unless defined($data->{$k1}{$k2});
			next unless (++$n > 1000);
			$data->{$SKIPPED_KEY}{$k2}++;
			$data->{$SKIPPED_SUM_KEY}{$k2} += $data->{$k1}{$k2};
			delete $data->{$k1}{$k2};
			$ndel++;
		}
	}
	foreach my $k1 (keys %$data) {
		delete $data->{$k1} unless (keys %{$data->{$k1}});
	}
	$ndel;
}
