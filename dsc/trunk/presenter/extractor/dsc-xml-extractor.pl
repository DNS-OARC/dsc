#!/usr/bin/env perl

# script must be run from /usr/local/dsc/data/$server/$node directory
#

use warnings;
use strict;

use DSC::extractor;
use DSC::extractor::config;
use Data::Dumper;

my $dbg = 0;

foreach my $fn (@ARGV) {
	extract($fn);
}

sub extract {
	my $xmlfile = shift || die "usage: $0 xmlfile\n";
	die "cant divine dataset" unless ($xmlfile =~ /\d+\.(\w+)\.xml$/);
	my $dataset = $1;
	print STDERR "dataset is $dataset\n" if ($dbg);

	my $EX = $DSC::extractor::config::DATASETS{$dataset};
	print STDERR 'EX=', Dumper($EX) if ($dbg);
	die "no extractor for $dataset\n" unless defined($EX);

	my $start_time;
	my $grokked;
	if ($EX->{ndim} == 1) {
		($start_time, $grokked) = grok_1d_xml($xmlfile, $EX->{type1});
	} elsif ($EX->{ndim} == 2) {
		($start_time, $grokked) = grok_2d_xml($xmlfile, $EX->{type1}, $EX->{type2});
	} else {
		die "unsupported ndim $EX->{ndim}\n";
	}
	my $yymmdd = &yymmdd($start_time);

	print STDERR 'grokked=', Dumper($grokked) if ($dbg);

	foreach my $output (keys %{$EX->{outputs}}) {
		print STDERR "output=$output\n" if ($dbg);
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
		print STDERR 'POST MUNGE grok_copy=', Dumper($grok_copy) if ($dbg);

		# read the current data file
		#
		exit(254) if (&{$O->{data_reader}}(\%db, "$yymmdd/$dataset/$output.dat") < 0);

		# merge/combine
		#
		&{$O->{data_merger}}($start_time, \%db, $grok_copy);
		print STDERR 'POST MERGE db=', Dumper(\%db) if ($dbg);

		# trim
		#
		if (defined($O->{data_trimer}) && ((59*60) == ($start_time % 3600))) {
			my $ntrim = &{$O->{data_trimer}}(\%db, $O);
			print STDERR "trimmed $ntrim records from $xmlfile\n" if ($ntrim);
		}

		# write out the new data file
		#
		&{$O->{data_writer}}(\%db, "$yymmdd/$dataset/$output.dat");

	}
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
	foreach my $k2 (@{$O->{keys}}) {
		my $n = 0;
		foreach my $k1 (sort {$data->{$b}{$k2} <=> $data->{$a}{$k2}} keys %$data) {
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
