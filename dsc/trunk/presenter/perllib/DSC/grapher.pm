package DSC::grapher;

use DSC::ploticus;
use DSC::extractor;
use DSC::grapher::template qw(default_binsize);
use DSC::grapher::config;

use POSIX;
use List::Util qw(max);
use Chart::Ploticus;
use CGI;
use CGI::Untaint;
use DBI;
use Data::Dumper;
use Digest::MD5;
use Text::Template;
use Hash::Merge;

use strict;
use warnings;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
		&run
		&cgi
        );
        %EXPORT_TAGS = ( );
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

# constants
my $dbg_lvl = 2;
my $DATAROOT = '/usr/local/dsc/data';
my $CacheImageTTL = 600; # 10 min
my @valid_tlds = qw(
ac ad ae af ag ai al am an ao aq ar as at au aw az ba bb bd be bf
bg bh bi bj bm bn bo br bs bt bv bw by bz ca cc cd cf cg ch ci ck
cl cm cn co cr cu cv cx cy cz de dj dk dm do dz ec ee eg eh er es
et fi fj fk fm fo fr ga gd ge gf gg gh gi gl gm gn gp gq gr gs gt
gu gw gy hk hm hn hr ht hu id ie il im in io iq ir is it je jm jo
jp ke kg kh ki km kn kp kr kw ky kz la lb lc li lk lr ls lt lu lv
ly ma mc md mg mh mk ml mm mn mo mp mq mr ms mt mu mv mw mx my mz
na nc ne nf ng ni nl no np nr nu nz om pa pe pf pg ph pk pl pm pn
pr ps pt pw py qa re ro ru rw sa sb sc sd se sg sh si sj sk sl sm
sn so sr st sv sy sz tc td tf tg th tj tk tm tn to tp tr tt tv tw
tz ua ug uk um us uy uz va vc ve vg vi vn vu wf ws ye yt yu za zm
zw aero biz com coop info museum name net org pro gov edu mil int
arpa .
);

my $use_data_uri;
my %ARGS;	# from CGI
my $CFG;	# from dsc-cfg.pl
my $PLOT;	# = $DSC::grapher::config::PLOTS{name}
my $ACCUM_TOP_N;
my $cgi;
my $now;

sub cgi { $cgi; }

sub run {

	# initialize vars
	$use_data_uri = 1;
	%ARGS = ();
	$CFG = ();	# from dsc-cfg.pl
	$PLOT = undef;
	$ACCUM_TOP_N = 40;
	$cgi = new CGI();
	$now = time;

	debug(1, "===> starting at " . strftime('%+', localtime($now)));
	debug(2, "Client is = $ENV{REMOTE_ADDR}:$ENV{REMOTE_PORT}");
	debug(3, "ENV=" . Dumper(\%ENV));
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
	$ARGS{key} = $untaint->extract(-as_printable => 'key')		|| undef;

	$PLOT = $DSC::grapher::config::PLOTS{$ARGS{plot}};
	error("Unknown plot type: $ARGS{plot}") unless (defined ($PLOT));
	debug(3, "PLOT=" . Dumper($PLOT));
	$dbg_lvl = $PLOT->{debugflag} if defined($PLOT->{debugflag});

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
		my @newkeys;
		my @newcolors;
		my @newnames;
		for (my $i = 0; $i<int(@{$PLOT->{keys}}); $i++) {
			next unless ($ARGS{key} eq ${$PLOT->{keys}}[$i]);
			push(@newkeys, ${$PLOT->{keys}}[$i]);
			push(@newcolors, ${$PLOT->{colors}}[$i]);
			push(@newnames, ${$PLOT->{names}}[$i]);
		}
		# kinda stupid to modify $PLOT
		$PLOT->{keys} = \@newkeys;
		$PLOT->{colors} = \@newcolors;
		$PLOT->{names} = \@newnames;
	} else {
		delete $ARGS{key};
	}


	debug(1, "ARGS=" . Dumper(\%ARGS));
	my $cache_name = cache_name($ARGS{server},
		$ARGS{node},
		$ARGS{plot},
		$ARGS{end} == $now ? 0 : $ARGS{end},
		$ARGS{window},
		$ARGS{binsize},
		$ARGS{mini},
		$ARGS{yaxis},
		$ARGS{key});

	$ACCUM_TOP_N = 20 if ($ARGS{mini});
	$CFG = read_config('/usr/local/dsc/etc/dsc-grapher.cfg');
	debug(3, 'CFG=' . Dumper($CFG));

	if ('html' eq $ARGS{content}) {
		if ($use_data_uri) {
			if (!reason_to_not_plot()) {
				debug(1, "no reason to not plot");
				if (!check_image_cache($cache_name)) {
					debug(1, "need to make cached image");
					make_image($cache_name);
				}
			}
		}
		my $source = "/usr/local/dsc/share/html/plot.page";
		my $t = Text::Template->new(
			TYPE => 'FILE',
			SOURCE => $source,
			DELIMITERS => ['[', ']']
		);
		error("Text::Template failed for plot.page") unless defined ($t);
		print $cgi->header(-type=>'text/html',-expires=>'+15m');
		my %vars_to_pass = (
			foo => "bar",
			use_data_uri => $use_data_uri,
			ARGS => \%ARGS,
			CFG => \$CFG,
			now => $now,
			cache_name => $cache_name,
		);
		print $t->fill_in(
			PACKAGE => 'DSC::grapher::template',
			HASH => \%vars_to_pass,
			);
	} else {
		make_image($cache_name) unless (!reason_to_not_plot() && check_image_cache($cache_name));
		if (-f cache_image_path($cache_name)) {
			print $cgi->header(-type=>'image/png',-expires=>'+15m');
			cat_image($cache_name);
		}
	}
	debug(1, "<=== finished at " . strftime('%+', localtime($now)));
}

sub reason_to_not_plot {
	return 'Please select a server' if ($ARGS{server} eq 'none');
	return 'Please select a plot' if ($ARGS{plot} eq 'none');
	my $PLOT = $DSC::grapher::config::PLOTS{$ARGS{plot}};
	return 'Please select a Query Attributes sub-item' if ($PLOT->{plot_type} eq 'none');
	undef;
}

sub make_image {
	my $cache_name = shift;
	my $start;	# script starting time
	my $stop;	# script ending time
	my $data;	# hashref holding the data to plot
	my $datafile;	# temp filename storing plot data
	my $cache_image_path = cache_image_path($cache_name);

	return unless defined($PLOT);
	return if ($PLOT->{plot_type} eq 'none');
	debug(1, "Plotting $ARGS{server} $ARGS{node} $ARGS{plot} $ARGS{end} $ARGS{window} $ARGS{binsize}");
	$start = time;
	$data = load_data();
	debug(5, 'data=' . Dumper($data));
	if (defined($PLOT->{munge_func})) {
		debug(1, "munging");
		$data = &{$PLOT->{munge_func}}($data);
	}
	if ($ARGS{yaxis} eq 'percent') {
		debug(1, "converting to percentage");
		$data = convert_to_percentage($data, $PLOT->{plot_type});
	}
	debug(5, 'data=' . Dumper($data));
	$datafile = plotdata_tmp($ARGS{plot});
	if ($PLOT->{plot_type} eq 'trace') {
		trace_data_to_tmpfile($data, $datafile);
		trace_plot($datafile, $ARGS{binsize}, $cache_image_path);
	} elsif ($PLOT->{plot_type} eq 'accum1d') {
		accum1d_data_to_tmpfile($data, $datafile);
		accum1d_plot($datafile, $ARGS{binsize}, $cache_image_path);
	} elsif ($PLOT->{plot_type} eq 'accum2d') {
		accum2d_data_to_tmpfile($data, $datafile);
		accum2d_plot($datafile, $ARGS{binsize}, $cache_image_path);
	} elsif ($PLOT->{plot_type} eq 'hist2d') {
		# like for qtype_vs_qnamelen
		hist2d_data_to_tmpfile($data, $datafile);
		hist2d_plot($datafile, $ARGS{binsize}, $cache_image_path);
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
		# check if first and last fall on different days (UTC)
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
			$PLOT->{keys} = [ @{$CFG->{servers}{$ARGS{server}}} ];
			$PLOT->{names} = [ @{$CFG->{servers}{$ARGS{server}}} ];
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
		$PLOT->{keys},
		$tf,
		$ARGS{binsize},
		$ARGS{end},
		$ARGS{window},
		$PLOT->{yaxes}{$ARGS{yaxis}}{divideflag});
	my $stop = time;
	debug(1, "writing tmpfile took %d seconds, %d lines",
		$stop-$start,
		$nl);
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
	debug(1, "writing tmpfile took %d seconds, %d lines",
		$stop-$start,
		$n);
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
			next unless (grep {$k2 eq $_} @{$PLOT->{keys}});
			$accum_sum{$k1} += $data->{$k1}{$k2};
		}
	}
	$n = 0;
	foreach my $k1 (sort {$accum_sum{$b} <=> $accum_sum{$a}} keys %accum_sum) {
		my @vals;
		foreach my $k2 (@{$PLOT->{keys}}) {
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
	debug(1, "writing tmpfile took %d seconds", $stop-$start);
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
	foreach my $k1 (@{$PLOT->{keys}}) {
		foreach my $k2 (keys %{$data->{$k1}}) {
			$min_k2 = $k2 if ($k2 < $min_k2);
			$max_k2 = $k2 if ($k2 > $max_k2);
			$sum += $data->{$k1}{$k2};
		}
	}
	my $sum2 = 0;
	my $p1 = 0;
	my $p2 = undef;
	# now calc %iles
	foreach my $k2 ($min_k2..$max_k2) {
		foreach my $k1 (@{$PLOT->{keys}}) {
			$sum2 += $data->{$k1}{$k2} || 0;
		}
		$p1 = $k2 if ($sum2 / $sum < 0.05);
		$p2 = $k2 if ($sum2 / $sum > 0.95);
		last if (defined($p1) && defined($p2));
	}
	
	foreach my $k2 ($p1..$p2) {
		my @vals;
		foreach my $k1 (@{$PLOT->{keys}}) {
			my $val;
			if (defined($data->{$k1}{$k2})) {
				$val = sprintf "%f", $data->{$k1}{$k2};
			} else {
				$val = 0;
			}
			push (@vals, $val);
		}
		print $tf join(' ', $k2, @vals), "\n";
	}
	close($tf);
	my $stop = time;
	debug(1, "writing tmpfile took %d seconds", $stop-$start);
	#system "cat $tf 1>&2";
}

sub time_descr {
	my $from_t = $ARGS{end} - $ARGS{window};
	my $to_t = $ARGS{end};
	if ($PLOT->{plot_type} =~ /^accum/) {
		$from_t += (86400 - ($ARGS{end} % 86400));
		$to_t += (86400 - ($ARGS{end} % 86400) - 1);
		$to_t = $now if ($to_t > $now);
	}
	my $from_ctime = strftime("%b %d, %Y, %T", gmtime($from_t));
	my $to_ctime = strftime("%b %d, %Y, %T UTC", gmtime($to_t));
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
	my $pngfile = shift;
	my $plotnames = $PLOT->{plotnames} || $PLOT->{names};
	my $ntypes = @$plotnames;
	my $start = time;

	ploticus_init("png", "$pngfile.new");
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
		-labelsarrayref => $plotnames,
		-colorsarrayref => $PLOT->{colors},
		-indexesarrayref => [0..$ntypes-1],
		-barwidth => 4.5 / ($ARGS{window} / $binsize),
	};

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
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub accum1d_plot {
	my $tf = shift;
	my $binsize = shift;
	my $pngfile = shift;
	my $plotnames = $PLOT->{plotnames} || $PLOT->{names};
	my $ntypes = @$plotnames;
	my $start = time;

	ploticus_init("png", "$pngfile.new");
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

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	unless ($ARGS{mini}) {
		for(my $i=0;$i<$ntypes;$i++) {
			Ploticus_legendentry({
				-label => $PLOT->{names}->[$i],
				-details => $PLOT->{colors}->[$i],
				-tag => $PLOT->{keys}->[$i],
			});
		}
	}
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub accum2d_plot {
	my $tf = shift;
	my $binsize = shift;
	my $pngfile = shift;
	my $plotnames = $PLOT->{plotnames} || $PLOT->{names};
	my $ntypes = @$plotnames;
	my $start = time;

	ploticus_init("png", "$pngfile.new");
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
		-labelsarrayref => $plotnames,
		-colorsarrayref => $PLOT->{colors},
		-keysarrayref => $PLOT->{keys},
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

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub hist2d_plot {
	my $tf = shift;
	my $binsize = shift;	# ignored
	my $pngfile = shift;
	my $plotnames = $PLOT->{plotnames} || $PLOT->{names};
	my $ntypes = @$plotnames;
	my $start = time;

	ploticus_init("png", "$pngfile.new");
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
		-label => 'FIXME',
	};
	my $bars_opts = {
		-labelsarrayref => $plotnames,
		-colorsarrayref => $PLOT->{colors},
		-keysarrayref => $PLOT->{keys},
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

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub error {
	my $msg = shift;
	print $cgi->header(-type=>'text/html',-expires=>'+15m');
	print "<h2>$0 ERROR</h2><p>$msg\n";
	die "$msg\n";
}

sub dumpdata {
	my $ref = shift;
	print $cgi->header(-type=>'text/plain',-expires=>'+15m');
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

# return 0 if we should generate a cached image
# return 1 if the cached image is useable
sub check_image_cache {
	my $prefix = shift;
	my @sb = stat(cache_image_path($prefix));
	return 0 unless (@sb);
	return 0 unless (time - $sb[9] < $CacheImageTTL);
	return 1;
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

sub valid_tld_filter {
        my $tld = shift;
        grep {$tld eq $_} @valid_tlds;
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

sub read_config {
	my $f = shift;
	my %C;
	open(F, $f) || die "$f: $!\n";
	while (<F>) {
		my @x = split;
		next unless @x;
		my $directive = shift @x;
		if ($directive eq 'server') {
			my $servername = shift @x;
			$C{servers}{$servername} = \@x;
		}
		if ($directive =~ /windows$/) {
			$C{$directive} = \@x;
		}
	}
	close(F);
	\%C;
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
