#!/usr/bin/env perl

# script must be run from /usr/local/dsc/data/$server/$node directory
#

use warnings;
use strict;

use Cwd;
use DSC::db;
use DSC::extractor;
use DSC::extractor qw($SKIPPED_KEY $SKIPPED_SUM_KEY);
use DSC::extractor::config;
use Data::Dumper;
use Proc::PID::File;
use POSIX qw(nice);
use XML::Simple;

my $MAX_FILES = 1000;
my $MAX_TIME = 270;

# NOTE it looks like PID file cannot be on NFS.  I get
# dsc-xml-extractor.pid already locked at Proc/PID/File.pm line 99
my $pid_basename = pid_basename('dsc-xml-extractor');
if (Proc::PID::File->running(dir => '/var/tmp', name => $pid_basename)) {
	warn "$pid_basename Already running!" if -t STDERR;
	exit(0);
}
nice(19);

my $DSCDIR = "/usr/local/dsc";
my $DATADIR = "$DSCDIR/data";
my $dbg = 0;
my $perfdbg = 0;
my $DBCACHE;
my $N = 0;

read_config("$DSCDIR/etc/dsc-extractor.cfg");

my ($server, $node) = get_server_node();
my $dbh = get_dbh || die;
$dbh->{RaiseError} = 1;
my $server_id = get_server_id($dbh, $server);
my $node_id = get_node_id($dbh, $server_id, $node);
$dbh->commit;
$dbh->disconnect;
$dbh = undef;
print STDERR "server_id = $server_id, node_id = $node_id\n" if $dbg;

foreach my $yyyymmdd (<incoming/????-??-??>) {
	process_xml_dir("$yyyymmdd/*.xml");
	rmdir $yyyymmdd;
	exit(0);
}
# backward compatibility.  look for XML files in $node dir if
# there was no incoming/yyyy-mm-dd dir
process_xml_dir('*.xml');
exit(0);

sub process_xml_dir {
    my $theGlob = shift;
    my $start = time;
    my $dbh = get_dbh || die;
    $dbh->{RaiseError} = 1;
    print "globbing $theGlob\n";
    foreach my $fn (glob $theGlob) {
	my $donefn = get_donefn($fn);
	if (-s $donefn) {
		print STDERR "removing duplicate $fn in ", `pwd`;
		unlink $fn || warn "$fn: $!";
		next;
	}
	if (-f $donefn) {
		print STDERR "removing empty $donefn in ", `pwd`;
		unlink $donefn || warn "$donefn: $!";
	}
	print "extract $fn\n";
	my $x = eval { extract($dbh, $fn); };
	unless (defined $x) {
		warn "extract died with '", $@, "'\n";
		Mkdir ("errors", 0755);
		#
		# perl's rename is a pain.  it doesn't work
		# across devices and maybe doesn't strip leading
		# directory components from the target name?
		#
		system "mv $fn errors || rm -f $fn";
		my ($V,$D,$F) = File::Spec->splitpath($fn);
		if (open (ERR, ">errors/$F.err")) {
			print ERR "extract died with '", $@, "'\n";
			close(ERR);
		}
		$dbh->rollback;
		next;
	}
	unless ($x > 0) {
		$dbh->rollback;
		next;
	}
	rename($fn, $donefn) or die "$fn -> $donefn: $!";
	$dbh->commit;
	last if ($MAX_FILES == $N++);
	last if (time - $start > $MAX_TIME);
    }
}

sub get_donefn {
	my $fn = shift;
	die unless ($fn =~ m@(\d+).([^\.]+).xml@);
	my $when = $1;
	my $type = $2;
	my $yymmdd = &yymmdd($when - 60);
	Mkdir ("$yymmdd", 0755); # where the .dat files go!
	Mkdir ("done", 0775);
	Mkdir ("done/$yymmdd", 0755);
	Mkdir ("done/$yymmdd/$type", 0755);
	return "done/$yymmdd/$type/$when.$type.xml";
}

sub get_server_node {
	my $cwd = getcwd;
	my @x = reverse split(/\//, $cwd);
	return ($x[1], $x[0]);
}

sub pid_basename {
	my $prog = shift;
	return join('-', get_server_node(), $prog);
}

sub read_db {
	my $O = shift;
	my $yymmdd = shift;
	my $dataset = shift;
	my $output = shift;
	my $db = $DBCACHE->{$yymmdd}->{$dataset}->{$output};
	return $db if $db;
	my %foo = ();
	exit(254) if (&{$O->{data_reader}}(\%foo, "$yymmdd/$output.dat") < 0);
	print STDERR "read $yymmdd/$dataset/$output.dat\n";
	return $DBCACHE->{$yymmdd}->{$dataset}->{$output} = \%foo;
}

sub extract($$) {
	mark(undef);
	my $dbh = shift;
	my $xmlfile = shift || die "usage: $0 xmlfile\n";
	die "cant divine dataset" unless ($xmlfile =~ /(\d+)\.(\w+)\.xml$/);
	my $file_time = $1;
	my $dataset = $2;
	print STDERR "dataset is $dataset\n" if ($dbg);

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

	my $XS = XML::Simple->new(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($xmlfile);

	# this is the old way -- one dataset per file
	#
	if ($dataset ne 'dscdata') {
		return extract_dataset($dbh, $XML, $dataset, $file_time);
	}

	# this is the new way of grouping all datasets
	# together into a single file
	#
	while (my ($k,$v) = each %{$XML->{array}}) {
		extract_dataset($dbh, $v, $k, $file_time) or die "dataset $k extraction failed";
	}
	return 1;
}

sub extract_dataset($$$) {
	my $dbh = shift;
	my $XML = shift;
	my $dataset = shift;
	my $file_time = shift;
	print STDERR "dataset is $dataset\n" if ($dbg);

	my $EX = $DSC::extractor::config::DATASETS{$dataset};
	print STDERR 'EX=', Dumper($EX) if ($dbg);
	unless (defined($EX)) {
		warn "no extractor for $dataset\n" unless defined($EX);
		return 1;
	}

	my $start_time;
	my $grokked;
	if ($EX->{ndim} == 1) {
		($start_time, $grokked) = grok_1d_xml($XML, $EX->{type1});
	} elsif ($EX->{ndim} == 2) {
		($start_time, $grokked) = grok_2d_xml($XML, $EX->{type1}, $EX->{type2});
	} else {
		die "unsupported ndim $EX->{ndim}\n";
	}
	mark("grokked $dataset XML content");
	# round start time down to start of the minute
	$start_time = int($start_time / 60) * 60;
	my $yymmdd = &yymmdd($start_time);

	print STDERR 'grokked=', Dumper($grokked) if ($dbg);

	foreach my $output (keys %{$EX->{outputs}}) {
		print STDERR "output=$output\n" if ($dbg);
		my $O =  $EX->{outputs}{$output};
		my %db;

		# transform the input into something for this particular
		# output format
		#
		my $grok_copy;
		if (defined($O->{data_munger})) {
			$grok_copy = &{$O->{data_munger}}($grokked, $O);
		} else {
			$grok_copy = $grokked;
		}
		print STDERR 'POST MUNGE grok_copy=', Dumper($grok_copy) if ($dbg);
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
		print STDERR 'POST MERGE db=', Dumper(\%db) if ($dbg);
		mark("merged $output");

		# trim
		#
		if (defined($O->{data_trimer}) && ((59*60) == ($start_time % 3600))) {
			my $ntrim = &{$O->{data_trimer}}(\%db, $O);
			print "trimmed $ntrim records from $yymmdd/$dataset/$output.dat\n" if ($ntrim);
			mark("trimmed $output");
		}

		# write out the new data file
		#
		&{$O->{data_writer}}(\%db, "$yymmdd/$output.dat");
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
	my $sth = $dbh->prepare("INSERT INTO loaded_files " .
		"(time, dataset, server_id, node_id) VALUES (?, ?, ?, ?)");
	$sth->execute($file_time, $dataset, $server_id, $node_id);

	return 1;
}

#
# basically a wrapper for elsify_unwanted_keys();
#
sub munge_elsify {
	my $input = shift;	# XML tree from input file
	my $O = shift;		# extractor->output structure
	# copy the input data
	my $copy;
	eval Data::Dumper->Dump([$input], ['copy']);
	if ($O->{keys}) {
		# A 1D dataset
		print STDERR "elsifiying 1D dataset\n" if ($dbg);
		elsify_unwanted_keys($copy, $O->{keys});
	} elsif ($O->{keys2}) {
		# A 2D dataset
		print STDERR "elsifiying 2D dataset\n" if ($dbg);
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

sub Mkdir {
	my $dir = shift;
	my $mode = shift;
	return if -d $dir;
	mkdir($dir, $mode) or die ("$dir: $!");
}

my $mark_start;
sub mark {
	return unless $perfdbg;
	my $msg = shift;
	unless ($msg) {
		$mark_start = Time::HiRes::gettimeofday;
		return;
	}
	printf STDERR "MARK %7.2f :: $msg\n",
		Time::HiRes::gettimeofday - $mark_start;
}
