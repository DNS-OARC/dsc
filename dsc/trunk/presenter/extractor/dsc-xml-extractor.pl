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
		exit(254) if (&{$O->{data_reader}}(\%db, "$yymmdd/$dataset/$output.tmp") < 0);

		# merge/combine
		#
		&{$O->{data_merger}}($start_time, \%db, $grok_copy);
		print STDERR 'POST MERGE db=', Dumper(\%db) if ($dbg);

		# write out the new data file
		#
		&{$O->{data_writer}}(\%db, "$yymmdd/$dataset/$output.tmp");

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
	elsify_unwanted_keys($copy, $O->{keys});
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
