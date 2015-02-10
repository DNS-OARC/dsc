#!/usr/bin/env perl

# script must be run from /usr/local/dsc/data/$server/$node directory
#

use warnings;
use strict;

use Cwd;
use DSC::extractor;
use DSC::extractor::config;
use Data::Dumper;
use Proc::PID::File;
use POSIX qw(nice);
use XML::Simple;
use File::Copy;

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

my $dbg = 0;
my $DBCACHE;
my $N = 0;

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
	my $x = eval { extract($fn); };
	unless (defined $x) {
		warn "extract died with '", $@, "'\n";
		Mkdir ("errors", 0755);
		File::Copy::move($fn, "errors") || unlink $fn;
		my ($V,$D,$F) = File::Spec->splitpath($fn);
		if (open (ERR, ">errors/$F.err")) {
			print ERR "extract died with '", $@, "'\n";
			close(ERR);
		}
		next;
	}
	next unless ($x > 0);
	rename($fn, $donefn) or die "$fn -> $donefn: $!";
	last if ($MAX_FILES == $N++);
	last if (time - $start > $MAX_TIME);
    }
}

sub get_donefn {
	my $fn = shift;
	die unless ($fn =~ m@(\d+).([^\.]+).xml@);
	my $when = $1;
	my $type = $2;
	my $yymmdd = DSC::extractor::yymmdd($when - 60);
	Mkdir ("$yymmdd", 0755); # where the .dat files go!
	Mkdir ("done", 0775);
	Mkdir ("done/$yymmdd", 0755);
	Mkdir ("done/$yymmdd/$type", 0755);
	return "done/$yymmdd/$type/$when.$type.xml";
}

sub pid_basename {
	my $prog = shift;
	my $cwd = getcwd;
	my @x = reverse split(/\//, $cwd);
	return join('-', $x[1], $x[0], $prog);
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

sub extract {
	my $xmlfile = shift || die "usage: $0 xmlfile\n";
	die "cant divine dataset" unless ($xmlfile =~ /\d+\.(\w+)\.xml$/);
	my $dataset = $1;
	print STDERR "dataset is $dataset\n" if ($dbg);

        my $XS = XML::Simple->new(searchpath => '.', forcearray => 1);
        my $XML = $XS->XMLin($xmlfile);

	# this is the old way -- one dataset per file
	#
	if ($dataset ne 'dscdata') {
		return extract_dataset($XML, $dataset);
	}

	# this is the new way of grouping all datasets
	# together into a single file
	#
	while (my ($k,$v) = each %{$XML->{array}}) {
		extract_dataset($v, $k) or die "dataset $k extraction failed";
	}
	return 1;
}

sub extract_dataset {
	my $XML = shift;
	my $dataset = shift;
	print STDERR "dataset is $dataset\n" if ($dbg);

	my $EX = $DSC::extractor::config::DATASETS{$dataset};
	print STDERR 'EX=', Dumper($EX) if ($dbg);
	unless (defined($EX)) {
		warn "no extractor for $dataset\n";
		return 1;
	}

	my $start_time;
	my $grokked;
	if ($EX->{ndim} == 1) {
		($start_time, $grokked) = DSC::extractor::grok_1d_xml($XML, $EX->{type1});
	} elsif ($EX->{ndim} == 2) {
		($start_time, $grokked) = DSC::extractor::grok_2d_xml($XML, $EX->{type1}, $EX->{type2});
	} else {
		die "unsupported ndim $EX->{ndim}\n";
	}
	# round start time down to start of the minute
	$start_time = int($start_time / 60) * 60;
	my $yymmdd = DSC::extractor::yymmdd($start_time);

	print STDERR 'grokked=', Dumper($grokked) if ($dbg);

	foreach my $output (keys %{$EX->{outputs}}) {
		print STDERR "output=$output\n" if ($dbg);
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
		print STDERR 'POST MUNGE grok_copy=', Dumper($grok_copy) if ($dbg);

		# read the current data file
		#
		my $dbref = read_db($O, $yymmdd, $dataset, $output);
		warn "read_db failed\n" unless $dbref;
		return 0 unless $dbref;

		# merge/combine
		#
		&{$O->{data_merger}}($start_time, $dbref, $grok_copy);
		print STDERR 'POST MERGE db=', Dumper($dbref) if ($dbg);

		# trim
		#
		if (defined($O->{data_trimer}) && ((59*60) == ($start_time % 3600))) {
			my $ntrim = &{$O->{data_trimer}}($dbref, $O);
			print "trimmed $ntrim records from $yymmdd/$dataset/$output.dat\n" if ($ntrim);
		}

		# write out the new data file
		#
		&{$O->{data_writer}}($dbref, "$yymmdd/$output.dat");

	}
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
		DSC::extractor::elsify_unwanted_keys($copy, $O->{keys});
	} elsif ($O->{keys2}) {
		# A 2D dataset
		print STDERR "elsifiying 2D dataset\n" if ($dbg);
		foreach my $k1 (keys %$copy) {
			DSC::extractor::elsify_unwanted_keys(\%{$copy->{$k1}}, $O->{keys2});
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
			next if ($k2 eq $DSC::extractor::SKIPPED_KEY);
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
		if ($k1 eq $DSC::extractor::SKIPPED_SUM_KEY) {
			next;
		} elsif ($k1 eq $DSC::extractor::SKIPPED_KEY) {
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
			if ($k2 eq $DSC::extractor::SKIPPED_SUM_KEY) {
				next;
			} elsif ($k2 eq $DSC::extractor::SKIPPED_KEY) {
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
			next if ($k1 eq $DSC::extractor::SKIPPED_KEY);		# dont delete this
			next if ($k1 eq $DSC::extractor::SKIPPED_SUM_KEY);	# dont delete this
			$data->{$DSC::extractor::SKIPPED_KEY}{$k2}++;
			$data->{$DSC::extractor::SKIPPED_SUM_KEY}{$k2} += $data->{$k1}{$k2};
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
