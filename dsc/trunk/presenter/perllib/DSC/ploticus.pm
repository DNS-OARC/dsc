package OARC::ploticus;

use Chart::Ploticus;
use Data::Dumper;
use POSIX;
use File::Temp qw(tempfile);

use strict;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
		&Ploticus_create_datafile
		&Ploticus_create_datafile_type2
		&Ploticus_getdata
		&Ploticus_areadef
		&Ploticus_bars_vstacked
		&Ploticus_bars
		&Ploticus_lines
		&Ploticus_lines_stacked
		&Ploticus_xaxis
		&Ploticus_yaxis
		&Ploticus_legend
		&Ploticus_categories
		&Ploticus_legendentry
		&Ploticus_annotate
		&window2increment
		&extract_server_from_datafile_path
		&extract_node_from_datafile_path
		&index_in_array
		&plotdata_tmp
	 );
        %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

my $plotdata_tmp = '/tmp/plotdataXXXXXXXXXXXXXX';
my $strftimefmt = '%D.%T';

sub plotdata_tmp {
	my $label = shift;
	my $obj;
	if (defined($label)) {
		$obj = new File::Temp(TEMPLATE => "/tmp/plotdata.$label.XXXXXXXXXXXXX");
	} else {
		$obj = new File::Temp(TEMPLATE => $plotdata_tmp);
	}
	$obj;
}

sub Ploticus_create_datafile {
	my $hashref = shift;
	my $keysarrayref = shift;
	my $FH = shift;
	my $time_bin_size = shift || 60;
	my $end = shift;
	my $window = shift;
	my $divideflag = shift;
	my %newhash;
	my $cutoff = $end - $window;
	$divideflag = 0 unless defined($divideflag);
	#
	# convert the original data into possibly larger bins
	#
	foreach my $fromkey (sort {$a <=> $b} keys %$hashref) {
		# note $fromkey is a time_t.
		next if ($fromkey < $cutoff);
		my $tokey = $fromkey - ($fromkey % $time_bin_size);
		foreach my $qt (@$keysarrayref) {
			next unless defined($$hashref{$fromkey}{$qt});
			$newhash{$tokey}{$qt} += $$hashref{$fromkey}{$qt};
			$newhash{$tokey}{$qt . '_COUNT'}++;
		}
	}
	#
	# if our dataset is empty, create some fake entries with zeros
	# so that ploticus doesn't puke
	#
	unless ((keys %newhash)) {
		foreach my $qt (@$keysarrayref) {
			$newhash{$cutoff}{$qt} = 0;
			$newhash{$cutoff}{$qt . '_COUNT'} = 1;
		}
	}
	#
	# now write the new data
	#
	my $nl = 0;
	my $DF = $divideflag ? 60 : 1;
	foreach my $tokey (sort {$a <=> $b} keys %newhash ) {
		my @v = ();
		foreach my $qt (@$keysarrayref) {
			push (@v, defined($newhash{$tokey}{$qt}) ? $newhash{$tokey}{$qt} / ($DF*$newhash{$tokey}{$qt . '_COUNT'}): '-');
		}
		print $FH join(' ', POSIX::strftime($strftimefmt, gmtime($tokey)), @v), "\n";
		$nl++;
	}
	close($FH);
	$nl;
}

sub Ploticus_create_datafile_type2 {
	my $hashref = shift;
	my $FH = shift;
	my $time_bin_size = shift || 60;
	my $window = shift;
	my %newhash;
	my $now = $main::now || time;
	my $cutoff = $now - $window;
	my %COUNT;
	#
	# convert the original data into possibly larger bins
	#
	foreach my $fromkey (sort {$a <=> $b} keys %$hashref) {
		# note $fromkey is a time_t.
		next if ($fromkey < $cutoff);
		my $tokey = $fromkey - ($fromkey % $time_bin_size);
		next unless defined($$hashref{$fromkey});
		$newhash{$tokey} += $$hashref{$fromkey};
		$COUNT{$tokey}++;
	}
	#
	# now write the new data
	#
	foreach my $tokey (sort {$a <=> $b} keys %newhash) {
		my $timestr = POSIX::strftime($strftimefmt, gmtime($tokey));
		print $FH $timestr, ' ', defined($newhash{$tokey}) ? $newhash{$tokey} / ($COUNT{$tokey}) : '-', "\n";
	}
	close($FH);
}

sub Ploticus_getdata {
	my $datafile = shift;
	P("#proc getdata");
	P("file: $datafile");
}


sub Ploticus_areadef{
	my $ropts = shift;
	P("#proc areadef");
	PO($ropts, 'title');
	PO($ropts, 'rectangle', '1 1 6 4');
	PO($ropts, 'xscaletype');
	my $window = $ropts->{-window};
	my $end = $ropts->{-end};
	if (defined($window)) {
		my $then = $end - $window;
	#	   $then -= ($then % &window2increment($window));
		my $range_begin = POSIX::strftime($strftimefmt, gmtime($then));
		my $range_end = POSIX::strftime($strftimefmt, gmtime($end));
		P("xrange: $range_begin $range_end");
	} elsif (defined($ropts->{-xstackfields})) {
		P("xautorange: datafield=$ropts->{-xstackfields} combomode=stack lowfix=0");
	} else {
		P("xautorange: datafield=1");
	}
	PO($ropts, 'yscaletype');
	if (defined($ropts->{-ystackfields})) {
		P("yautorange: datafield=$ropts->{-ystackfields} combomode=stack lowfix=0");
	}
}

sub Ploticus_bars_vstacked { Ploticus_bars(shift); }

sub Ploticus_bars {
	my $ropts = shift;

	foreach my $i (@{$ropts->{-indexesarrayref}}) {
		my $field = $i+2;
		P("#proc bars");
		P('outline: no');
		P("lenfield: $field");
		PO($ropts, 'horizontalbars');
		PO($ropts, 'locfield', '1');
		PO($ropts, 'stackfields', '*');
		PO($ropts, 'barwidth');
		if (defined($ropts->{-exactcolorfield})) {
			PO($ropts, 'exactcolorfield');
		} elsif (defined($ropts->{-colorfield})) {
			PO($ropts, 'colorfield');
		} else {
			P("color: ${$ropts->{-colorsarrayref}}[$i]");
		}
		if (defined($ropts->{-labelsarrayref})) {
			my $legendlabel;
			# generate clickmap entries for the legend based on
			# a printf-like template
			if (defined($ropts->{-legend_clickmapurl_tmpl})) {
				my $URI = $ropts->{-legend_clickmapurl_tmpl};
				$URI =~ s/\@LEGEND\@/${$ropts->{-labelsarrayref}}[$i]/;
				$URI =~ s/\@KEY\@/${$ropts->{-keysarrayref}}[$i]/;
				$legendlabel .= "url:$URI ";
			}
			$legendlabel .= ${$ropts->{-labelsarrayref}}[$i];
			P("legendlabel: $legendlabel");
		}
		PO($ropts, 'clickmapurl');
	}
	PO($ropts, 'labelfield');
	P("labelzerovalue: yes") if defined($ropts->{-labelfield});
}

sub Ploticus_lines {
	my $ropts = shift;

	foreach my $i (@{$ropts->{-indexesarrayref}}) {
		my $field = $i+2;
		P("#proc lineplot");
		PO($ropts, 'xfield', '1');
		P("yfield: $field");
		P("linedetails: color=${$ropts->{-colorsarrayref}}[$i]");
		if (defined($ropts->{-labelsarrayref})) {
			P("legendlabel: ${$ropts->{-labelsarrayref}}[$i]");
		}
	}
	P("gapmissing: yes");
}

sub Ploticus_lines_stacked {
	my $cloneref = shift;
	my $labelsarrayref = shift;
	my $colorsarrayref = shift;
	my $indexesarrayref = shift;
	my $field;
	foreach my $i (@$indexesarrayref) {
		my $field = $i+2;
		P("#proc bars");
		&$cloneref if defined($cloneref);
		P("lenfield: $field");
		P("color: $$colorsarrayref[$i]");
		P("legendlabel: $$labelsarrayref[$i]");
	}
}

sub Ploticus_xaxis {
	my $ropts = shift;
	my $window = $ropts->{-window};
	P("#proc xaxis");
	if (!defined($window)) {
		P("stubs: inc");
	} elsif ($window >= 3*24*3600) {
		P("stubs: inc 1 day");
		P("stubformat: Mmmdd");
		P("label: Date");
	} elsif ($window > 8*3600) {
		if (defined($ropts->{-mini})) {
		P("stubs: inc 4 hours");
		} else {
		P("stubs: inc 2 hours");
		}
		P("autodays: yes");
		P("stubformat: hh:mm");
		P("label: Time, UTC");
	} elsif ($window > 2*3600) {
		P("stubs: inc 30 minutes");
		P("stubformat: hh:mm");
		P("label: Time, UTC");
	} else {
		P("stubs: inc 10 minutes");
		P("stubformat: hh:mm");
		P("label: Time, UTC");
	}
	PO($ropts, 'label');
	PO($ropts, 'grid');
	PO($ropts, 'stubcull');
}

sub Ploticus_yaxis{
	my $ropts = shift;
	P("#proc yaxis");
	PO($ropts, 'stubs', 'inc');
	PO($ropts, 'grid');
	PO($ropts, 'label');
}

sub Ploticus_legend {
	my $ropts = shift;
	P("#proc legend");
	PO($ropts, 'location', 'max+0.5 max');
	PO($ropts, 'reverseorder', 'yes');
	P("outlinecolors: yes");
}

sub Ploticus_categories {
	my $catfield = shift;
	P("#proc categories");
	P("axis: y");
	P("datafield: $catfield");
}

sub Ploticus_legendentry {
	my $ropts = shift;
	P("#proc legendentry");
	P("sampletype: color");
	PO($ropts, 'label');
	PO($ropts, 'details');
	PO($ropts, 'tag');
}

sub Ploticus_annotate {
	my $ropts = shift;
	P("#proc annotate");
	PO($ropts, 'textdetails');
	PO($ropts, 'location');
	PO($ropts, 'text');
	P("");
}

sub window2increment {
	my $window = shift;
	return 10*60 if ($window == 3600);
	return 30*60 if ($window == 4*3600);
	return 2*3600 if ($window == 24*3600);
	return 24*3600 if ($window == 7*24*3600);
	warn "window2increment: bad window value $window";
	undef;
}

sub extract_server_from_datafile_path {
	my $fn = shift;
	die "$fn" unless ($fn =~ m@/([^/]+)/[^/]+/\d\d\d\d\d\d\d\d/@);
	return $1;
}

sub extract_node_from_datafile_path {
	my $fn = shift;
	die "$fn" unless ($fn =~ m@/[^/]+/([^/]+)/\d\d\d\d\d\d\d\d/@);
	return $1;
}

sub index_in_array {
	my $arrayref = shift;
	my $val = shift;
	for(my $i=0; $i<@$arrayref; $i++) {
		return $i if ($$arrayref[$i] eq $val);
	}
	-1;
}

sub PO {
	my $ropts = shift;
	my $optname = shift;
	my $default = shift;
	if (defined ($ropts->{-$optname})) {
		P("$optname: $ropts->{-$optname}");
	} elsif (defined($default)) {
		P("$optname: $default");
	}
}

sub P {
	my $line = shift;
	print STDERR "$line\n" if ($main::ploticus_debug);
	Chart::Ploticus::ploticus_execline($line);
}

1;

