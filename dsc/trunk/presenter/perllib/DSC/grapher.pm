package DSC::grapher;

use DSC::ploticus;
use DSC::extractor;
use DSC::grapher::plot;
use DSC::grapher::text;
use DSC::grapher::config;

use POSIX;
use List::Util qw(max);
use CGI;
use CGI::Untaint;
use Data::Dumper;
use Digest::MD5;
use Text::Template;
use Hash::Merge;
use Math::Calc::Units;

use strict;
use warnings;

BEGIN {
	use Exporter ();
	use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
	$VERSION = 1.00;
	@ISA	 = qw(Exporter);
	@EXPORT	 = qw(
		&prepare
		&run
		&cgi
	);
	%EXPORT_TAGS = ( );
	@EXPORT_OK   = qw();
}
use vars @EXPORT;
use vars @EXPORT_OK;

END { }

# constants
my $dbg_lvl = 2;
my $DATAROOT = '/usr/local/dsc/data';
my $CacheImageTTL = 60; # 1 min
my $expires_time = '+1m';

my $use_data_uri;
my %ARGS;	# from CGI
my $CFG;	# from dsc-cfg.pl
my $PLOT;	# = $DSC::grapher::plot::PLOTS{name}
my $TEXT;	# = $DSC::grapher::text::TEXTS{name}
my $ACCUM_TOP_N;
my $cgi;
my $now;

# we have to make this damn hack for mod_perl, which makes it unsafe
# to modify $PLOT
#
my @plotkeys;
my @plotcolors;
my @plotnames;

sub prepare {
	# initialize vars
	$use_data_uri = 1;
	%ARGS = ();
	$CFG = ();	# from dsc-cfg.pl
	$PLOT = undef;
	$ACCUM_TOP_N = 40;
	$cgi = new CGI();
	$now = time;
}

sub cgi { $cgi; }


sub run {
	my $cfgfile = shift || '/usr/local/dsc/etc/dsc-grapher.cfg';
	# read config file early so we can set back the clock if necessary
	#
	$CFG = DSC::grapher::config::read_config($cfgfile);
	debug(3, 'CFG=' . Dumper($CFG)) if ($dbg_lvl >= 3);
	$now -= $CFG->{embargo} if defined $CFG->{embargo};

	debug(1, "===> starting at " . POSIX::strftime('%+', localtime($now)));
	debug(2, "Client is = $ENV{REMOTE_ADDR}:$ENV{REMOTE_PORT}");
	debug(3, "ENV=" . Dumper(\%ENV)) if ($dbg_lvl >= 3);
	my $untaint = CGI::Untaint->new($cgi->Vars);
	$ARGS{server} = $untaint->extract(-as_printable => 'server')	|| 'none';
	$ARGS{node} = $untaint->extract(-as_printable => 'node')	|| 'all';
	$ARGS{window} = $untaint->extract(-as_integer => 'window')	|| 3600*4;
	$ARGS{binsize} = $untaint->extract(-as_integer => 'binsize')	|| default_binsize($ARGS{window});
	$ARGS{plot} = $untaint->extract(-as_printable => 'plot')	|| 'bynode';
	$ARGS{content} = $untaint->extract(-as_printable => 'content')	|| 'html';
	$ARGS{mini} = $untaint->extract(-as_integer => 'mini')		|| 0;
	$ARGS{end} = $untaint->extract(-as_integer => 'end')		|| $now;
	$ARGS{yaxis} = $untaint->extract(-as_printable => 'yaxis')	|| undef;
	$ARGS{key} = $untaint->extract(-as_printable => 'key');		# sanity check below

	$PLOT = $DSC::grapher::plot::PLOTS{$ARGS{plot}};
	$TEXT = $DSC::grapher::text::TEXTS{$ARGS{plot}};
	error("Unknown plot type: $ARGS{plot}") unless (defined ($PLOT));
	error("Unknown server: $ARGS{server}") unless ('none' eq $ARGS{server} || defined ($CFG->{servers}{$ARGS{server}}));
	error("Unknown node: $ARGS{node}") unless ('all' eq $ARGS{node} || (grep {$_ eq $ARGS{node}} @{$CFG->{servers}{$ARGS{server}}}));
	error("Time window cannot be larger than a month") if ($ARGS{window} > 86400*31);
	debug(3, "PLOT=" . Dumper($PLOT)) if ($dbg_lvl >= 3);
	$dbg_lvl = $PLOT->{debugflag} if defined($PLOT->{debugflag});

	@plotkeys = @{$PLOT->{keys}};
	@plotnames = @{$PLOT->{names}};
	@plotcolors = @{$PLOT->{colors}};

	# Sanity checking on CGI args
	#
	if (!defined($ARGS{yaxis})) {
		$ARGS{yaxis} = find_default_yaxis_type();
	} elsif (!defined($PLOT->{yaxes}{$ARGS{yaxis}})) {
		$ARGS{yaxis} = find_default_yaxis_type();
	}

	if ($PLOT->{plot_type} =~ /^accum/ && $ARGS{window} < 86400) {
		$ARGS{window} = 86400;
	}

	$ARGS{end} = $now if ($ARGS{end} > $now);

	if (defined($ARGS{key}) && grep {$_ eq $ARGS{key}} @{$PLOT->{keys}}) {
		@plotkeys = ();
		@plotnames = ();
		@plotcolors = ();
		for (my $i = 0; $i<int(@{$PLOT->{keys}}); $i++) {
			next unless ($ARGS{key} eq ${$PLOT->{keys}}[$i]);
			push(@plotkeys, ${$PLOT->{keys}}[$i]);
			push(@plotnames, ${$PLOT->{names}}[$i]);
			push(@plotcolors, ${$PLOT->{colors}}[$i]);
		}
	} else {
		delete $ARGS{key};
	}


	debug(1, "ARGS=" . Dumper(\%ARGS));
	my $cache_name = cache_name($ARGS{server},
		$ARGS{node},
		$ARGS{plot} . ($CFG->{anonymize_ip} ? '_anon' : ''),
		$ARGS{end},
		$ARGS{window},
		$ARGS{binsize},
		$ARGS{mini},
		$ARGS{yaxis},
		$ARGS{key});

	$ACCUM_TOP_N = 20 if ($ARGS{mini});

	if ('html' eq $ARGS{content}) {
		if (!reason_to_not_plot()) {
			debug(1, "no reason to not plot");
			if (!check_image_cache($cache_name)) {
				debug(1, "need to make cached image");
				make_image($cache_name);
			}
		}
		my $source = "/usr/local/dsc/share/html/plot.page";
		my $t = Text::Template->new(
			TYPE => 'FILE',
			SOURCE => $source,
			DELIMITERS => ['[', ']']
		);
		error("Text::Template failed for plot.page") unless defined ($t);
		print $cgi->header(-type=>'text/html',-expires=>$expires_time)
			unless (defined($CFG->{'no_http_header'}));
		my %vars_to_pass = (
			navbar_servers_nodes => navbar_servers_nodes(),
			navbar_plot => navbar_plot(),
			navbar_window => navbar_window(),
			navbar_yaxis => navbar_yaxis(),
			img_with_map => img_with_map($cache_name),
			description => $TEXT->{description},
		);
		print $t->fill_in(
			PACKAGE => 'DSC::grapher::template',
			HASH => \%vars_to_pass,
			);
	} else {
		make_image($cache_name) unless (!reason_to_not_plot() && check_image_cache($cache_name));
		if (-f cache_image_path($cache_name)) {
			print $cgi->header(-type=>'image/png',-expires=>$expires_time);
			cat_image($cache_name);
		}
	}
	debug(1, "<=== finished at " . POSIX::strftime('%+', localtime($now)));
}

sub reason_to_not_plot {
	return 'Please select a server' if ($ARGS{server} eq 'none');
	return 'Please select a plot' if ($ARGS{plot} eq 'none');
	my $PLOT = $DSC::grapher::plot::PLOTS{$ARGS{plot}};
	return 'Please select a Query Attributes sub-item' if ($PLOT->{plot_type} eq 'none');
	undef;
}

sub make_image {
	my $cache_name = shift;
	my $start;	# script starting time
	my $stop;	# script ending time
	my $data;	# hashref holding the data to plot
	my $datafile;	# temp filename storing plot data

	return unless defined($PLOT);
	return if ($PLOT->{plot_type} eq 'none');
	debug(1, "Plotting $ARGS{server} $ARGS{node} $ARGS{plot} $ARGS{end} $ARGS{window} $ARGS{binsize}");
	$start = time;
	$data = load_data();
	debug(5, 'data=' . Dumper($data)) if ($dbg_lvl >= 5);
	if (defined($PLOT->{munge_func})) {
		debug(1, "munging");
		$data = &{$PLOT->{munge_func}}($data);
	}
	if ($ARGS{yaxis} eq 'percent') {
		debug(1, "converting to percentage");
		$data = convert_to_percentage($data, $PLOT->{plot_type});
	}
	debug(5, 'data=' . Dumper($data)) if ($dbg_lvl >= 5);
	$datafile = plotdata_tmp($ARGS{plot});
	if ($PLOT->{plot_type} eq 'trace') {
		trace_data_to_tmpfile($data, $datafile) and
		trace_plot($datafile, $ARGS{binsize}, $cache_name);
	} elsif ($PLOT->{plot_type} eq 'accum1d') {
		accum1d_data_to_tmpfile($data, $datafile) and
		accum1d_plot($datafile, $ARGS{binsize}, $cache_name);
	} elsif ($PLOT->{plot_type} eq 'accum2d') {
		accum2d_data_to_tmpfile($data, $datafile) and
		accum2d_plot($datafile, $ARGS{binsize}, $cache_name);
	} elsif ($PLOT->{plot_type} eq 'hist2d') {
		# like for qtype_vs_qnamelen
		hist2d_data_to_tmpfile($data, $datafile) and
		hist2d_plot($datafile, $ARGS{binsize}, $cache_name);
	} else {
		error("Unknown plot type: $PLOT->{plot_type}");
	}
	$stop = time;
	debug(1, "graph took %d seconds", $stop-$start);
}

sub datafile_path {
	my $plot = shift;
	my $node = shift;
	my $when = shift;
	my $dataset = $PLOT->{dataset} || $plot;
	my $datafile = $PLOT->{datafile} || $plot;
	join('/',
		$DATAROOT,
		$ARGS{server},
		$node,
		yymmdd($when),
		$dataset,
		"$datafile.dat");
}

sub load_data {
	my $datadir;
	my %hash;
	my $start = time;
	my $last = $ARGS{end};
	my $first = $ARGS{end} - $ARGS{window};
	my $nl = 0;
	if ($PLOT->{plot_type} =~ /^accum/) {
		#
		# for 'accum' plots, round up the start time
		# to the beginning of the next 24hour period
		#
		$first += (86400 - ($ARGS{end} % 86400));
	}
	if (($last - $first < 86400) && yymmdd($last) ne yymmdd($first)) {
		# check if first and last fall on different days (per $TZ)
		# and the window is less than a day.  If so, we need
		# to make sure that the datafile that 'last' lies in
		# gets read.
		$last = $first + 86400;
	}
	foreach my $node (@{$CFG->{servers}{$ARGS{server}}}) {
	    next if ($ARGS{node} ne 'all' && $node ne $ARGS{node});
	    for (my $t = $first; $t <= $last; $t += 86400) {
		my %thash;
		my $datafile = datafile_path($ARGS{plot}, $node, $t);
		debug(1, "reading $datafile");
		#warn "$datafile: $!\n" unless (-f $datafile);
		if ('bynode' eq $ARGS{plot}) {
			# XXX ugly special case
			$nl += &{$PLOT->{data_reader}}(\%thash, $datafile);
			bynode_summer(\%thash, \%hash, $node);
			# XXX yes, wasteful repetition
			@plotkeys = @{$CFG->{servers}{$ARGS{server}}};
			@plotnames = @{$CFG->{servers}{$ARGS{server}}};
		} else {
			# note that 'summing' is important for 'accum'
			# plots, even for a single node
			$nl += &{$PLOT->{data_reader}}(\%thash, $datafile);
			&{$PLOT->{data_summer}}(\%thash, \%hash);
		}
	    }
	}
	my $stop = time;
	debug(1, "reading datafile took %d seconds, %d lines",
		$stop-$start,
		$nl);
	\%hash;
}

sub trace_data_to_tmpfile {
	my $data = shift;
	my $tf = shift;
	my $start = time;
	my $nl = Ploticus_create_datafile($data,
		\@plotkeys,
		$tf,
		$ARGS{binsize},
		$ARGS{end},
		$ARGS{window},
		$PLOT->{yaxes}{$ARGS{yaxis}}{divideflag});
	my $stop = time;
	debug(1, "writing tmpfile took %d seconds, %d lines",
		$stop-$start,
		$nl);
	$nl
}

# calculate the amount of time in an 'accum' dataset.
#
sub calc_accum_win {
	my $last = $ARGS{end};
	my $first = $ARGS{end} - $ARGS{window};
	$first += (86400 - ($ARGS{end} % 86400));
	debug(1, "accum window = %.2f days", ($last - $first) / 86400);
	$last - $first;
}

sub accum1d_data_to_tmpfile {
	my $data = shift;
	my $tf = shift;
	my $start = time;
	my $accum_win = $PLOT->{yaxes}{$ARGS{yaxis}}{divideflag} ?
		calc_accum_win() : 1;
	my $n;
	delete $data->{$SKIPPED_KEY};
	delete $data->{$SKIPPED_SUM_KEY};
	$n = 0;
	foreach my $k1 (sort {$data->{$b} <=> $data->{$a}} keys %$data) {
		print $tf join(' ',
			$k1,
			$data->{$k1} / $accum_win,
			&{$PLOT->{label_func}}($k1),
			&{$PLOT->{color_func}}($k1),
			), "\n";
		last if (++$n == $ACCUM_TOP_N);
	}
	close($tf);
	my $stop = time;
	debug(1, "writing $n lines to tmpfile took %d seconds, %d lines",
		$stop-$start,
		$n);
	$n;
}

sub accum2d_data_to_tmpfile {
	my $data = shift;
	my $tf = shift;
	my $start = time;
	my %accum_sum;
	my $accum_win = $PLOT->{yaxes}{$ARGS{yaxis}}{divideflag} ?
		calc_accum_win() : 1;
	my $n;
	delete $data->{$SKIPPED_KEY};
	delete $data->{$SKIPPED_SUM_KEY};
	foreach my $k1 (keys %$data) {
		foreach my $k2 (keys %{$data->{$k1}}) {
			next unless (grep {$k2 eq $_} @plotkeys);
			$accum_sum{$k1} += $data->{$k1}{$k2};
		}
	}
	$n = 0;
	foreach my $k1 (sort {$accum_sum{$b} <=> $accum_sum{$a}} keys %accum_sum) {
		my @vals;
		foreach my $k2 (@plotkeys) {
			my $val;
			if (defined($data->{$k1}{$k2})) {
				$val = sprintf "%f", $data->{$k1}{$k2} / $accum_win;
			} else {
				$val = 0;
			}
			push (@vals, $val);
		}
		push(@vals, $accum_sum{$k1});	# label location (?)
		print $tf join(' ', $k1, @vals), "\n";
		last if (++$n == $ACCUM_TOP_N);
	}
	close($tf);
	my $stop = time;
	debug(1, "writing $n lines to tmpfile took %d seconds", $stop-$start);
	$n;
}

sub hist2d_data_to_tmpfile {
	my $data = shift;
	my $tf = shift;
	my $start = time;
	delete $data->{$SKIPPED_KEY};
	delete $data->{$SKIPPED_SUM_KEY};
	
	# find 5th,95th percentile for auto x-axis scaling
	my $min_k2 = 1000000;
	my $max_k2 = 0;
	my $sum = 0;
	# first find min,max k2 range, and calc sum
	foreach my $k1 (@plotkeys) {
		foreach my $k2 (keys %{$data->{$k1}}) {
			$min_k2 = $k2 if ($k2 < $min_k2);
			$max_k2 = $k2 if ($k2 > $max_k2);
			$sum += $data->{$k1}{$k2};
		}
	}
	return 0 if ($max_k2 < $min_k2);
	my $sum2 = 0;
	my $p1 = 0;
	my $p2 = undef;
	# now calc %iles
	foreach my $k2 ($min_k2..$max_k2) {
		foreach my $k1 (@plotkeys) {
			$sum2 += $data->{$k1}{$k2} || 0;
		}
		$p1 = $k2 if ($sum2 / $sum < 0.05);
		$p2 = $k2 if ($sum2 / $sum > 0.95);
		last if (defined($p1) && defined($p2));
	}
	
	my $nl = 0;
	foreach my $k2 ($p1..$p2) {
		my @vals;
		foreach my $k1 (@plotkeys) {
			my $val;
			if (defined($data->{$k1}{$k2})) {
				$val = sprintf "%f", $data->{$k1}{$k2};
			} else {
				$val = 0;
			}
			push (@vals, $val);
		}
		print $tf join(' ', $k2, @vals), "\n";
		$nl++;
	}
	close($tf);
	my $stop = time;
	debug(1, "writing $nl lines to tmpfile took %d seconds", $stop-$start);
	#system "cat $tf 1>&2";
	$nl;
}

sub time_descr {
	my $from_t = $ARGS{end} - $ARGS{window};
	my $to_t = $ARGS{end};
	if ($PLOT->{plot_type} =~ /^accum/) {
		$from_t += (86400 - ($ARGS{end} % 86400));
		$to_t += (86400 - ($ARGS{end} % 86400) - 1);
		$to_t = $now if ($to_t > $now);
	}
	my $from_ctime = POSIX::strftime("%b %d, %Y, %T", localtime($from_t));
	my $to_ctime = POSIX::strftime("%b %d, %Y, %T %Z", localtime($to_t));
	"From $from_ctime To $to_ctime";
}

sub find_default_yaxis_type {
	foreach my $t (keys %{$PLOT->{yaxes}}) {
		return $t if ($PLOT->{yaxes}{$t}{default});
	}
	#die "[$$] no default yaxis type for $ARGS{plot}\n";
	'none';
}

sub trace_plot {
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = cache_image_path($cache_name);
	my $ntypes = @plotnames;
	my $start = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($PLOT->{map_legend}) {
		$mapfile = cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_arg("-maxrows", "20000");
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	#system "cat $tf 1>&2";

	my $areadef_opts = {
		-title => $PLOT->{plottitle} . "\n" . time_descr(),
		-rectangle => '1 1 6 4',
		-xscaletype => 'datetime mm/dd/yy',
		-ystackfields => join(',', 2..($ntypes+1)),
		-end => $ARGS{end},
		-window => $ARGS{window},
	};
	my $xaxis_opts = {
		-window => $ARGS{window},
		-stubcull => '0.5',
	};
	my $yaxis_opts = {
		-label => $PLOT->{yaxes}{$ARGS{yaxis}}{label},
		-grid => 'yes',
	};
	my $bars_opts = {
		-labelsarrayref => \@plotnames,
		-colorsarrayref => \@plotcolors,
		-indexesarrayref => [0..$ntypes-1],
		-keysarrayref => \@plotkeys,
		-barwidth => 4.5 / ($ARGS{window} / $binsize),
	};
	if (defined($mapfile)) {
		my %copy = %ARGS;
		delete $copy{key};
		my $uri = urlpath(%copy);
		$uri .= '&key=@KEY@';
		debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}

	if ($ARGS{mini}) {
		$areadef_opts->{-title} = $PLOT->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 4 2';
		delete $yaxis_opts->{-label};
		$xaxis_opts->{-mini} = 'yes';
	}

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub accum1d_plot {
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = cache_image_path($cache_name);
	my $ntypes = @plotnames;
	my $start = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($PLOT->{map_legend}) {
		$mapfile = cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	Ploticus_categories(1);
	my $areadef_opts = {
		-title => $PLOT->{plottitle} . "\n" . time_descr(),
		-rectangle => '1 1 6 6',
		-yscaletype => 'categories',
		-xstackfields => '2',
	};
	my $xaxis_opts = {
		# yes we abuse/confuse x/y axes
		-label => $PLOT->{yaxes}{$ARGS{yaxis}}{label},
		-grid => 'yes',
	};
	my $yaxis_opts = {
		-stubs => 'usecategories',
	};
	my $bars_opts = {
		-colorfield => '4',
		-indexesarrayref => [0],
		-labelfield => 3,
		-horizontalbars => 'yes',
	};
	my $legend_opts = {
		-reverseorder => 'no',
	};

	if ($ARGS{mini}) {
		$areadef_opts->{-title} = $PLOT->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 3 4';
		delete($xaxis_opts->{-label});
	}

	if (defined($mapfile)) {
		my %copy = %ARGS;
		delete $copy{key};
		my $uri = urlpath(%copy);
		$uri .= '&key=@KEY@';
		debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}
 
	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	unless ($ARGS{mini}) {
		for(my $i=0;$i<$ntypes;$i++) {
			Ploticus_legendentry({
				-label => $plotnames[$i],
				-details => $plotcolors[$i],
				-tag => $plotkeys[$i],
			});
		}
	}
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub accum2d_plot {
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = cache_image_path($cache_name);
	my $ntypes = @plotnames;
	my $start = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($PLOT->{map_legend}) {
		$mapfile = cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	Ploticus_categories(1);
	my $areadef_opts = {
		-title => $PLOT->{plottitle} . "\n" . time_descr(),
		-rectangle => '1 1 6 6',
		-yscaletype => 'categories',
		-xstackfields => join(',', 2..($ntypes+1)),
	};
	my $xaxis_opts = {
		# yes we abuse/confuse x/y axes
		-label => $PLOT->{yaxes}{$ARGS{yaxis}}{label},
		-grid => 'yes',
	};
	my $yaxis_opts = {
		-stubs => 'usecategories',
	};
	my $bars_opts = {
		-labelsarrayref => \@plotnames,
		-colorsarrayref => \@plotcolors,
		-keysarrayref => \@plotkeys,
		-indexesarrayref => [0..$ntypes-1],
		-horizontalbars => 'yes',
	};
	my $legend_opts = {
		-reverseorder => 'no',
	};

	if ($ARGS{mini}) {
		$areadef_opts->{-title} = $PLOT->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 3 4';
		delete($xaxis_opts->{-label});
	}

	if (defined($mapfile)) {
		my %copy = %ARGS;
		delete $copy{key};
		my $uri = urlpath(%copy);
		$uri .= '&key=@KEY@';
		debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}
 
	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub hist2d_plot {
	my $tf = shift;
	my $binsize = shift;	# ignored
	my $cache_name = shift;
	my $pngfile = cache_image_path($cache_name);
	my $ntypes = @plotnames;
	my $start = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($PLOT->{map_legend}) {
		$mapfile = cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	Ploticus_categories(1);
	my $areadef_opts = {
		-title => $PLOT->{plottitle} . "\n" . time_descr(),
		-rectangle => '1 1 6 4',
		-ystackfields => join(',', 2..($ntypes+1)),
	};
	my $yaxis_opts = {
		-label => $PLOT->{yaxes}{$ARGS{yaxis}}{label},
		-grid => 'yes',
	};
	my $xaxis_opts = {
		-label => $PLOT->{xaxislabel},
	};
	my $bars_opts = {
		-labelsarrayref => \@plotnames,
		-colorsarrayref => \@plotcolors,
		-keysarrayref => \@plotkeys,
		-indexesarrayref => [0..$ntypes-1],
	};
	my $legend_opts = {
		-reverseorder => 'no',
	};

	if ($ARGS{mini}) {
		$areadef_opts->{-title} = $PLOT->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 3 4';
		delete($xaxis_opts->{-label});
	}

	if (defined($mapfile)) {
		my %copy = %ARGS;
		delete $copy{key};
		my $uri = urlpath(%copy);
		$uri .= '&key=@KEY@';
		debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub error {
	my $msg = shift;
	print $cgi->header(-type=>'text/html',-expires=>$expires_time);
	print "<h2>$0 ERROR</h2><p>$msg\n";
	exit(1);
}

sub dumpdata {
	my $ref = shift;
	print $cgi->header(-type=>'text/plain',-expires=>$expires_time);
	print Dumper($ref);
}



sub munge_2d_to_1d {
	# this function changes a 2D array into a 1D array
	# by combining key values.   For example $dat{k1}{k2} becomes $dat{"k1:k2"}
	my $data = shift;
	my $d1keys = shift;
	my $d2keys = shift;
	my %newdata;
	my $j1;
	my $j2;
	my $N = 0;
	foreach my $t (keys %$data) {
		foreach my $k1 (keys %{$data->{$t}}) {
			$j1 = $k1;
			#$j1 = 'else' unless grep {$_ eq $j1} @$d1keys;
			next unless grep {$_ eq $j1} @$d1keys;
			foreach my $k2 (keys %{$data->{$t}{$k1}}) {
				$j2 = $k2;
				#$j2 = 'else' unless grep {$_ eq $j2} @$d2keys;
				next unless grep {$_ eq $j2} @$d2keys;
				$newdata{$t}{"$j1:$j2"} += $data->{$t}{$k1}{$k2};
			}
		}
	}
	\%newdata;
}

sub munge_anonymize_ip {
	# anonymize IP addresses, leaving only 1st octet.
	# since anonymizing may result in colissions, we sum the values
	my $data = shift;
	return $data unless ($CFG->{anonymize_ip});
	my %newdata;
	foreach my $k1 (keys %$data) {
		next if ($k1 eq $SKIPPED_KEY);
		next if ($k1 eq $SKIPPED_SUM_KEY);
		my @f = split(/\./, $k1);
		my $newkey = join('.', $f[0], 0, 0, 0);
		if ('HASH' eq ref($data->{$k1})) {
			# assume 2d
			foreach my $k2 (keys %{$data->{$k1}}) {
				$newdata{$newkey}{$k2} += $data->{$k1}{$k2};
			}
		} else {
			# assume 1d
			$newdata{$newkey} += $data->{$k1};
		}
	}
	\%newdata;
}

sub convert_to_percentage {
	my $data = shift;
	my $type = shift;
	my %newdata;
	my $S;
	if ('trace' eq $type) {
	    foreach my $t (keys %$data) {
		$S = 0;
		foreach my $k1 (keys %{$data->{$t}}) {
			$S += $data->{$t}{$k1};
		}
		next unless ($S > 0);
		$S /= 100;
		foreach my $k1 (keys %{$data->{$t}}) {
			$newdata{$t}{$k1} = $data->{$t}{$k1} / $S;
		}
	    }
	} elsif ('accum2d' eq $type || 'hist2d' eq $type) {
		$S = 0;
		foreach my $k1 (keys %$data) {
			foreach my $k2 (keys %{$data->{$k1}}) {
				$S += $data->{$k1}{$k2};
			}
		}
		$S /= 100;
		foreach my $k1 (keys %$data) {
			foreach my $k2 (keys %{$data->{$k1}}) {
				$newdata{$k1}{$k2} = $data->{$k1}{$k2} / $S;
			}
		}
	} elsif ('accum1d' eq $type) {
		$S = 0;
		foreach my $k1 (keys %$data) {
			$S += $data->{$k1};
		}
		$S /= 100;
		foreach my $k1 (keys %$data) {
			$newdata{$k1} = $data->{$k1} / $S;
		}
	} else {
		die "dont know how to percentify plot type $type";
	}
	\%newdata;
}

sub cache_name {
	my $ctx = Digest::MD5->new;
	$ctx->add(grep {defined($_)} @_);
	$ctx->hexdigest;
}

sub cache_image_path {
	my $prefix = shift || die;
	"/usr/local/dsc/cache/$prefix.png";
}

sub cache_mapfile_path {
	my $prefix = shift || die;
	"/usr/local/dsc/cache/$prefix.map";
}

# return 0 if we should generate a cached image
# return 1 if the cached image is useable
sub check_image_cache {
	my $prefix = shift;
	my $f = cache_image_path($prefix);
	my @sb = stat($f);
	return 0 unless (@sb);
	unless (time - $sb[9] < $CacheImageTTL) {
		unlink $f;
		return 0;
	}
	return 1;
}

sub cached_image_size {
	my $prefix = shift;
	my @sb = stat(cache_image_path($prefix));
	return -1 unless (@sb);
	$sb[7];
}

sub cat_image {
	my $prefix = shift;
	if (open(F, cache_image_path($prefix))) {
		print while (<F>);
		close(F);
	} else {
		#die "$prefix: $!\n";
	}
}

sub mapfile_to_buf {
	my $prefix = shift || die;
	my $buf = '';
	if (open(F, cache_mapfile_path($prefix))) {
		$buf .= $_ while (<F>);
		close(F);
	} else {
		#die "$prefix: $!\n";
	}
	$buf;
}

sub image_to_buf {
	my $prefix = shift;
	my $buf = '';
	if (open(F, cache_image_path($prefix))) {
		$buf .= $_ while (<F>);
		close(F);
	} else {
		#die "$prefix: $!\n";
	}
	$buf;
}

my @valid_domains = ();

sub valid_tld_filter {
	my $tld = shift;
	@valid_domains = DSC::grapher::config::get_valid_domains($ARGS{server})
		unless @valid_domains;
	grep {$_ && $tld eq $_} @valid_domains;
}

sub numeric_tld_filter {
	my $tld = shift;
	return ($tld =~ /^[0-9]+$/) ? 1 : 0;
}

sub invalid_tld_filter {
	my $tld = shift;
	return 0 if (valid_tld_filter($tld));
	return 0 if (numeric_tld_filter($tld));
	return 1;
}

sub data_summer_0d {
	my $from = shift;
	my $to = shift;
	my $start = time;
	foreach my $k0 (keys %$from) {
		$to->{$k0} += $from->{$k0};
	}
	my $stop = time;
	debug(1, "data_summer_0d took %d seconds", $stop-$start);
}

sub data_summer_1d {
	my $from = shift;
	my $to = shift;
	my $start = time;
	foreach my $k0 (keys %$from) {
		foreach my $k1 (keys %{$from->{$k0}}) {
			$to->{$k0}{$k1} += $from->{$k0}{$k1};
		}
	}
	my $stop = time;
	debug(1, "data_summer_1d took %d seconds", $stop-$start);
}

sub data_summer_2d {
	my $from = shift;
	my $to = shift;
	my $start = time;
	foreach my $k0 (keys %$from) {
		foreach my $k1 (keys %{$from->{$k0}}) {
			foreach my $k2 (keys %{$from->{$k0}{$k1}}) {
				$to->{$k0}{$k1}{$k2} += $from->{$k0}{$k1}{$k2};
			}
		}
	}
	my $stop = time;
	debug(2, "data_summer_2d took %d seconds", $stop-$start);
}

# XXX special hack for "bynode" plots
# XXX assume $from hash is 1D
# NOTE this is very similar to data_summer_1d
#
sub bynode_summer {
	my $from = shift;
	my $to = shift;
	my $newkey = shift;
	my $start = time;
	foreach my $k0 (keys %$from) {
		foreach my $k1 (keys %{$from->{$k0}}) {
			$to->{$k0}{$newkey} += $from->{$k0}{$k1};
		}
	}
	my $stop = time;
	debug(2, "bynode_summer took %d seconds", $stop-$start);
}

sub debug {
	my $l = shift;
	if ($dbg_lvl >= $l) {
		print STDERR "[$$] ";
		printf STDERR @_;
		print STDERR "\n";
	}
}

1;


##########################


use URI::Escape;
use MIME::Base64;
use Carp;
use Data::Dumper;

my %IconData;

sub img_with_map {
	my $cache_name = shift;
	my $imgsrc = '';
	my $result = '';

	my %attrs;

	if (my $reason = reason_to_not_plot()) {
		return "<div class=\"notice\">$reason</div>";
	}

	unless (cached_image_size($cache_name) > 0) {
		return "<div class=\"notice\">No Data To Display At This Time</div>";
	}

	my $pn = $ARGS{plot};
	my $withmap = ($DSC::grapher::plot::PLOTS{$pn}->{map_legend}) ? 1 : 0;
	if ($withmap) {
		$result .= html_markup('map', {name=>'TheMap'}, mapfile_to_buf($cache_name));
		$attrs{usemap} = '#TheMap';
	}
	
	if ($use_data_uri) {
		$imgsrc = "data:image/png;base64,\n";
		$imgsrc .= encode_base64(image_to_buf($cache_name));
	} else {
		my %own_args = %ARGS;	# copy
		while (my $k = shift) { $own_args{$k} = shift; }
		$own_args{content} = 'image';
		$imgsrc = urlpath(%own_args);
	}
	$attrs{src} = $imgsrc;
	$result .= html_markup('img', \%attrs, undef);
	$result;
}

sub urlpath {
	my %args = @_;
	my $cgi = $ENV{REQUEST_URI};
	if ((my $n = index($cgi,'?')) > 0) {
		$cgi = substr($cgi,0,$n);
	}
	delete_default_args(\%args);
	"$cgi?" . join('&', map {"$_=" . uri_escape($args{$_})} keys %args);
}

sub merge_args {
	my %new = @_;
	my %old = %ARGS;
	foreach my $k (keys %new) {
		$old{$k} = $new{$k};
	}
	%old;
}

sub default_binsize {
	my $win = shift || (3600*4);
	my $bin = $win / 240;
	$bin = 60 if ($bin < 60);
	int($bin);
}

sub delete_default_args {
	my $href = shift;
	my $pn = $ARGS{plot};
	unless (defined($pn)) {
		carp "oops";
	}
	delete $href->{server} if ('none' eq $href->{server});
	delete $href->{node} if ('all' eq $href->{node});
	delete $href->{binsize} if (default_binsize($ARGS{window}) == $href->{binsize});
	delete $href->{binsize} unless ($DSC::grapher::plot::PLOTS{$pn}->{plot_type} eq 'trace');
	delete $href->{end} if ((abs($now - $href->{end}) / $href->{window}) < 0.20);
	delete $href->{window} if (3600*4 == $href->{window});
	delete $href->{plot} if ('none' eq $href->{plot});
	delete $href->{content} if ('html' eq $href->{content});
	delete $href->{mini} if (0 == $href->{mini});
	delete $href->{yaxis} if (find_default_yaxis_type() eq $href->{yaxis});
}

sub server { $ARGS{server}; }
sub node { $ARGS{node}; }

sub a_markup {
	my $h = shift;
	my $a = shift;
	my $c = shift;
	my $t = shift;
	die "no h" unless (defined($h));
	die "no a" unless (defined($a));
	html_markup('a',
		{ class => $c, href => $h, title => $t },
		$a);
}

sub img_markup {
	my $s = shift;
	my $c = shift;
	my $a = shift;
	if ($use_data_uri) {
		$IconData{$s} = load_icon_data($s) unless defined($IconData{$s});
		my $t;
		$t = "data:image/png;base64,\n";
		$t .= encode_base64($IconData{$s});
		$s = $t;
	} else {
		$s = "/dsc/$s";
	}
	html_markup('img',
		{ class => $c, src => $s, alt => $a },
		undef);
}

sub navbar_item {
	my $arg = shift;
	my $val = shift;
	my $label = shift;
	my $class = ($val eq $ARGS{$arg}) ? 'current' : undef;
	my %newargs;
	$newargs{$arg} = $val;
	$newargs{node} = 'all' if ($arg eq 'server');
	a_markup(urlpath(merge_args(%newargs)), $label, $class) . "\n";
}

sub navbar_arrow_item {
	my $delta = shift;
	my $icon = shift;
	my $alt = shift;
	a_markup(urlpath(merge_args(end=>$ARGS{end} + $delta)),
		img_markup("$icon.png", undef, $alt),
		undef,	# class
		$alt);	# title
}

sub sublist_item { '&rsaquo;&nbsp;'; }

sub navbar_servers_nodes {
	my $snippet = '';
	$snippet .= "<ul>\n";
	my @items;
	foreach my $server ( keys %{$CFG->{servers}} ) {
		#print STDERR "server=$server\n";
		$snippet .= "<li>" . navbar_item('server',$server,$server);
		if ($ARGS{server} eq $server) {
			foreach my $node (@{$CFG->{servers}{$server}}) {
				$snippet .= '<li>' . sublist_item();
				$snippet .= navbar_item('node',$node,$node);
			}
		}
	}
	$snippet .= "</ul>\n";
	$snippet;
}

sub navbar_plot {
	my @items = ();
	my $pn = $ARGS{plot} || die;
	push(@items, navbar_item('plot','bynode','By Node')) if ($ARGS{node} eq 'all');
	push(@items, navbar_item('plot','qtype','Qtypes'));
	if ($pn eq 'qtype' || $pn eq 'dnssec_qtype') {
		push(@items, sublist_item() . navbar_item('plot','dnssec_qtype','DNSSEC Qtypes'));
	}
	push(@items, navbar_item('plot','rcode','Rcodes'));
	push(@items, navbar_item('plot','client_subnet2_accum','Classification'));
	if ($pn =~ /^client_subnet2/) {
		push(@items, sublist_item() . navbar_item('plot','client_subnet2_trace', 'trace'));
		push(@items, sublist_item() . navbar_item('plot','client_subnet2_count', 'count'));
	}
	push(@items, navbar_item('plot','client_subnet_accum','Client Geography'));
	push(@items, navbar_item('plot','qtype_vs_all_tld','TLDs'));
	if ($pn =~ /qtype_vs_.*_tld/) {
		push(@items, sublist_item() . navbar_item('plot','qtype_vs_valid_tld', 'valid'));
		push(@items, sublist_item() . navbar_item('plot','qtype_vs_invalid_tld', 'invalid'));
		push(@items, sublist_item() . navbar_item('plot','qtype_vs_numeric_tld', 'numeric'));
	}
	push(@items, navbar_item('plot','client_addr_vs_rcode_accum','Rcodes by Client Address'));
	push(@items, navbar_item('plot','certain_qnames_vs_qtype','Popular Names'));
	push(@items, navbar_item('plot','ipv6_rsn_abusers_accum','IPv6 root abusers'));
	push(@items, navbar_item('plot','opcode','Opcodes'));
	push(@items, navbar_item('plot','query_attrs','Query Attributes'));
	if ($pn =~ /query_attrs|idn_qname|rd_bit|do_bit|edns_version/) {
		push(@items, sublist_item() . navbar_item('plot','idn_qname', 'IDN Qnames'));
		push(@items, sublist_item() . navbar_item('plot','rd_bit', 'RD bit'));
		push(@items, sublist_item() . navbar_item('plot','do_bit', 'DO bit'));
		push(@items, sublist_item() . navbar_item('plot','edns_version', 'EDNS version'));
	}
	push(@items, navbar_item('plot','chaos_types_and_names','CHAOS'));
	push(@items, navbar_item('plot','direction_vs_ipproto','IP Protocols'));
	push(@items, navbar_item('plot','qtype_vs_qnamelen','Qname Length'));
	push(@items, navbar_item('plot','rcode_vs_replylen','Reply Lengths'));
	"<ul>\n" . join('<li>', '', @items) . "</ul>\n";
}

# This is function called from an "HTML" file by the template parser
#
sub navbar_window {
	my @items = ();
	my $pn = $ARGS{plot};
	my $PLOT = $DSC::grapher::plot::PLOTS{$pn};
	if (defined($PLOT->{plot_type}) && $PLOT->{plot_type} =~ /^accum/) {
		foreach my $w (@{$CFG->{accum_windows}}) {
			push(@items, navbar_item('window',units_to_seconds($w),$w));
		}
		my @arrows;
		push(@arrows, navbar_arrow_item(-$ARGS{window}, '1leftarrow',
			"Backward " . seconds_to_units($ARGS{window})));
		push(@arrows, navbar_arrow_item($ARGS{window}, '1rightarrow',
			"Forward " . seconds_to_units($ARGS{window})));
		push(@items, html_markup("div", {class=>'center'}, join("&nbsp;&nbsp;", @arrows)));
	} else {
		# trace
		foreach my $w (@{$CFG->{trace_windows}}) {
			push(@items, navbar_item('window',units_to_seconds($w),$w));
		}
		my @arrows;
		push(@arrows, navbar_arrow_item(-$ARGS{window}, '2leftarrow',
			"Backward " . seconds_to_units($ARGS{window})));
		push(@arrows, navbar_arrow_item(-$ARGS{window}/2, '1leftarrow',
			"Backward " . seconds_to_units($ARGS{window}/2)));
		push(@arrows, navbar_arrow_item($ARGS{window}/2, '1rightarrow',
			"Forward " . seconds_to_units($ARGS{window}/2)));
		push(@arrows, navbar_arrow_item($ARGS{window}, '2rightarrow',
			"Forward " . seconds_to_units($ARGS{window})));
		push(@items, html_markup("div", {class=>'center'}, join("&nbsp;&nbsp;", @arrows)));
	}
	join('<br>', @items);
}


# This is function called from an "HTML" file by the template parser
#
sub navbar_yaxis {
	my @items = ();
	my $pn = $ARGS{plot};
	my $PLOT = $DSC::grapher::plot::PLOTS{$pn};
	foreach my $t (keys %{$PLOT->{yaxes}}) {
		push(@items, navbar_item('yaxis',$t, $PLOT->{yaxes}{$t}{label}));
	}
	join('<br>', @items);
}

sub units_to_seconds {
	my $win = shift;
	die unless ($win =~ /(\d+)(\w+)/);
	my $n = $1;
	my $unit = $2;
	if ($unit =~ /^hour/) {
		$n *= 3600;
	} elsif ($unit =~ /^day/) {
		$n *= 86400;
	} elsif ($unit =~ /^week/) {
		$n *= 7*86400;
	} elsif ($unit =~ /^month/) {
		$n *= 30*86400;
	} elsif ($unit =~ /^year/) {
		$n *= 365*86400;
	} else {
		die "unknown unit: $unit";
	}
	$n;
}

sub seconds_to_units {
	my $s = shift;
	my $a;
	my $v;
	foreach my $u qw(years weeks days hours minutes) {
		$a = Math::Calc::Units::convert("$s seconds", $u);
		$a =~ /^([-\.\de]+)/ or die "bad units $a";
		$v = $1;
		last if ($v >= 1);
	}
	$a =~ s/s$// if ($v == 1);
	$a;
}

sub load_icon_data {
	my $icon = shift;	# should be like 'foo.png"
	my $buf;
	if (open(F, "/usr/local/dsc/share/html/$icon")) {
		$buf .= $_ while (<F>);
		close(F);
	} else {
		warn "/usr/local/dsc/htdocs/$icon: $!\n";
	}
	$buf;
}

sub html_markup {
	my $tag = shift;
	my $attrs = shift;
	my $cdata = shift;
	my $buf = '';
	$buf .= "<$tag";
	while (my($k,$v) = each %$attrs) {
		next unless defined($v);
		$buf .= " $k=" . '"' . $v . '"';
	}
	$buf .= ">";
	if (defined($cdata)) {
		$buf .= $cdata;
		$buf .= "</$tag>";
	}
	$buf;
}
1;
