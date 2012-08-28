package DSC::grapher;

use DSC::ploticus;
use DSC::extractor;
use DSC::grapher::plot;
use DSC::grapher::text;
use DSC::grapher::config;

use Carp;
use POSIX;
use List::Util qw(max);
use CGI;
use CGI::Untaint;
use Data::Dumper;
use Digest::MD5;
use Text::Template;
use Hash::Merge;
use Math::Calc::Units;
use Switch;

use strict;
use warnings;

BEGIN { }

END { }

# CONSTANTS
my $dbg_lvl = 0;	# also set debug_file in dsc-grapher.cfg
my $DATAROOT = '/usr/local/dsc/data';
my $DEFAULTCONFIG = '/usr/local/dsc/etc/dsc-grapher.cfg';
my $CacheImageTTL = 60;		# 1 min
my $expires_time = '+1m';
my $sublist_item ='&rsaquo;&nbsp;';
my @argnames = qw ( server node window binsize plot content mini end yaxis key );

sub new {
	my $class = shift;
	my %config = @_;
	my $self = {};
	# initialize vars
	$self->{PLOT} = undef;	# = $DSC::grapher::plot::PLOTS{name}
	$self->{TEXT} = undef;	# = $DSC::grapher::text::TEXTS{name}
	$self->{ACCUM_TOP_N} = 40;
	$self->{cgi} = undef;
	$self->{now} = time;
	$self->{plotkeys} = ();
	$self->{plotcolors} = ();
	$self->{plotnames} = ();
	$self->{valid_domains} = ();
	#
	# read config file early so we can set back the clock if necessary
	#
	my $cfgfile = $config{configfile} || $DEFAULTCONFIG;
	$self->{CFG} = DSC::grapher::config::read_config($cfgfile);
	$self->{now} -= $self->{CFG}->{embargo} if defined $self->{CFG}->{embargo};
	$dbg_lvl = $self->{CFG}->{debug_level} if defined $self->{CFG}->{debug_level};
	#
	# set up default args
	#
	$self->{ARGS}->{server} = 'none';
	$self->{ARGS}->{node} = 'all';
	$self->{ARGS}->{window} = 3600*4;
	$self->{ARGS}->{binsize} = default_binsize($self->{ARGS}->{window});
	$self->{ARGS}->{plot} = 'bynode';
	$self->{ARGS}->{content} = 'png';
	$self->{ARGS}->{mini} = 0;
	$self->{ARGS}->{end} = $self->{now};
	$self->{ARGS}->{yaxis} = undef;
	$self->{ARGS}->{key} = undef;
	#
	#
	#
	bless($self, $class);
	return $self;
}

sub cmdline {
	my $self = shift;
	my $args = shift;
	foreach my $k (@argnames) {
		next unless defined $args->{$k};
		$self->{ARGS}->{$k} = $args->{$k};
		if ('window' eq $k && !defined($args->{binsize})) {
			$self->{ARGS}->{binsize} = default_binsize($args->{window});
		}
	}
	$self->process_args;
}

sub cgi {
	my $self = shift;
	if (@_) {
		$self->{cgi} = shift;
		$self->debug(2, "Client is = $ENV{REMOTE_ADDR}:$ENV{REMOTE_PORT}");
		$self->debug(3, "ENV=" . Dumper(\%ENV)) if ($dbg_lvl >= 3);
		my $untaint = CGI::Untaint->new($self->{cgi}->Vars);
		$self->{ARGS}->{server} = $untaint->extract(-as_printable => 'server') if defined $self->{cgi}->param('server');
		$self->{ARGS}->{node} = $untaint->extract(-as_printable => 'node') if defined $self->{cgi}->param('node');
		$self->{ARGS}->{window} = $untaint->extract(-as_integer => 'window') if defined $self->{cgi}->param('window');
		$self->{ARGS}->{binsize} = $untaint->extract(-as_integer => 'binsize') if defined $self->{cgi}->param('binsize');
		$self->{ARGS}->{plot} = $untaint->extract(-as_printable => 'plot') if defined $self->{cgi}->param('plot');
		$self->{ARGS}->{mini} = $untaint->extract(-as_integer => 'mini') if defined $self->{cgi}->param('mini');
		$self->{ARGS}->{end} = $untaint->extract(-as_integer => 'end') if defined $self->{cgi}->param('end');
		$self->{ARGS}->{yaxis} = $untaint->extract(-as_printable => 'yaxis') if defined $self->{cgi}->param('yaxis');
		$self->{ARGS}->{key} = $untaint->extract(-as_printable => 'key') if defined $self->{cgi}->param('key');
		$self->{ARGS}->{content} = $untaint->extract(-as_printable => 'content') || 'html';
		$self->{use_data_uri} = defined $self->{cgi}->param('x') ? 1 : 0;
		$self->process_args;
	}
	return $self->{cgi};
}

sub process_args {
	my $self = shift;
	$self->debug(3, 'CFG=' . Dumper($self->{CFG})) if ($dbg_lvl >= 3);
	$self->{PLOT} = $DSC::grapher::plot::PLOTS{$self->{ARGS}->{plot}};
	$self->{TEXT} = $DSC::grapher::text::TEXTS{$self->{ARGS}->{plot}};
	$self->error("Unknown plot type: $self->{ARGS}->{plot}") unless (defined ($self->{PLOT}));
	$self->error("Unknown server: $self->{ARGS}->{server}") unless ('none' eq $self->{ARGS}->{server} || defined ($self->{CFG}->{servers}{$self->{ARGS}->{server}}));
	$self->error("Unknown node: $self->{ARGS}->{node}") unless ('all' eq $self->{ARGS}->{node} || (grep {$_ eq $self->{ARGS}->{node}} @{$self->{CFG}->{servers}{$self->{ARGS}->{server}}}));
	$self->error("Time window cannot be larger than a month") if ($self->{ARGS}->{window} > 86400*31);
	$dbg_lvl = $self->{PLOT}->{debugflag} if defined($self->{PLOT}->{debugflag});
	$self->debug(3, "PLOT=" . Dumper($self->{PLOT})) if ($dbg_lvl >= 3);

	@{$self->{plotkeys}} = @{$self->{PLOT}->{keys}};
	@{$self->{plotnames}} = @{$self->{PLOT}->{names}};
	@{$self->{plotcolors}} = @{$self->{PLOT}->{colors}};

	# Sanity checking on args
	#
	if (!defined($self->{ARGS}->{yaxis})) {
		$self->{ARGS}->{yaxis} = $self->find_default_yaxis_type();
	} elsif (!defined($self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}})) {
		$self->{ARGS}->{yaxis} = $self->find_default_yaxis_type();
	}

	if ($self->{PLOT}->{plot_type} =~ /^accum/ && $self->{ARGS}->{window} < 86400) {
		$self->{ARGS}->{window} = 86400;
	}

	$self->{ARGS}->{end} = $self->{now} if ($self->{ARGS}->{end} > $self->{now});

	if (defined($self->{ARGS}->{key}) && grep {$_ eq $self->{ARGS}->{key}} @{$self->{PLOT}->{keys}}) {
		@{$self->{plotkeys}} = ();
		@{$self->{plotnames}} = ();
		@{$self->{plotcolors}} = ();
		for (my $i = 0; $i<int(@{$self->{PLOT}->{keys}}); $i++) {
			next unless ($self->{ARGS}->{key} eq ${$self->{PLOT}->{keys}}[$i]);
			push(@{$self->{plotkeys}}, ${$self->{PLOT}->{keys}}[$i]);
			push(@{$self->{plotnames}}, ${$self->{PLOT}->{names}}[$i]);
			push(@{$self->{plotcolors}}, ${$self->{PLOT}->{colors}}[$i]);
		}
	} else {
		delete $self->{ARGS}->{key};
	}
}

sub run {
	my $self = shift;
	$self->debug(1, "===> starting at " . POSIX::strftime('%+', localtime($self->{now})));
	$self->debug(1, "ARGS=" . Dumper($self->{ARGS}));
	my $cache_name = $self->cache_name($self->{ARGS}->{server},
		$self->{ARGS}->{node},
		$self->{ARGS}->{plot} . ($self->{CFG}->{anonymize_ip} ? '_anon' : ''),
		$self->{ARGS}->{end},
		$self->{ARGS}->{window},
		$self->{ARGS}->{binsize},
		$self->{ARGS}->{mini},
		$self->{ARGS}->{yaxis},
		$self->{ARGS}->{key});

	$self->{ACCUM_TOP_N} = 20 if ($self->{ARGS}->{mini});

	if ('html' eq $self->{ARGS}->{content}) {
		if (!$self->reason_to_not_plot) {
			$self->debug(1, "no reason to not plot");
			if (!$self->check_image_cache($cache_name)) {
				$self->debug(1, "need to make cached image");
				$self->make_image($cache_name);
			}
		}
		my $source = "/usr/local/dsc/share/html/plot.page";
		my $t = Text::Template->new(
			TYPE => 'FILE',
			SOURCE => $source,
			DELIMITERS => ['[', ']']
		);
		$self->error("Text::Template failed for plot.page") unless defined ($t);
		print $self->cgi->header(-type=>'text/html',-expires=>$expires_time)
			unless (defined($self->{CFG}->{'no_http_header'}));
		my %vars_to_pass = (
			navbar_servers_nodes => $self->navbar_servers_nodes,
			navbar_plot => $self->navbar_plot,
			navbar_window => $self->navbar_window,
			navbar_yaxis => $self->navbar_yaxis,
			img_with_map => $self->img_with_map($cache_name),
			description => $self->{TEXT}->{description},
		);
		print $t->fill_in(
			PACKAGE => 'DSC::grapher::template',
			HASH => \%vars_to_pass,
			);
	} else {
		$self->make_image($cache_name) unless (!$self->reason_to_not_plot && $self->check_image_cache($cache_name));
		if (-f $self->cache_image_path($cache_name) && $self->cgi ) {
			print $self->cgi->header(-type=>'image/png',-expires=>$expires_time)
			    unless (defined($self->{CFG}->{'no_http_header'}));
			$self->cat_image($cache_name);
		}
	}
	$self->debug(1, "<=== finished at " . POSIX::strftime('%+', localtime($self->{now})));
}

sub reason_to_not_plot {
	my $self = shift;
	return 'Please select a server' if ($self->{ARGS}->{server} eq 'none');
	return 'Please select a plot' if ($self->{ARGS}->{plot} eq 'none');
	#XXX
	#$self->{PLOT} = $DSC::grapher::plot::PLOTS{$self->{ARGS}->{plot}};
	return 'Please select an Attributes sub-item' if ($self->{PLOT}->{plot_type} eq 'none');
	undef;
}

sub make_image {
	my $self = shift;
	my $cache_name = shift;
	my $start_t;	# script starting time
	my $stop_t;	# script ending time
	my $data;	# hashref holding the data to plot
	my $datafile;	# temp filename storing plot data

	return unless defined($self->{PLOT});
	return if ($self->{PLOT}->{plot_type} eq 'none');
	$self->debug(1, "Plotting $self->{ARGS}->{server} $self->{ARGS}->{node} $self->{ARGS}->{plot} $self->{ARGS}->{end} $self->{ARGS}->{window} $self->{ARGS}->{binsize}");
	$start_t = time;
	$data = $self->load_data;
	$self->debug(5, 'loaded data: ' . Dumper($data)) if ($dbg_lvl >= 5);
	$data = $self->munge_data($data);
	$self->debug(5, 'munged data: ' . Dumper($data)) if ($dbg_lvl >= 5);
	$datafile = plotdata_tmp($self->{ARGS}->{plot});
	$self->write_data($datafile, $data) and
	$self->plot_data($datafile, $cache_name);
	$stop_t = time;
	$self->debugf(1, "graph took %d seconds", $stop_t-$start_t);
}

sub datafile_path {
	my $self = shift;
	my $plot = shift;
	my $server = shift;
	my $node = shift;
	my $when = shift;
	my $datafile = $self->{PLOT}->{datafile} || $self->{PLOT}->{dataset} || $self->{ARGS}->{plot};
	join('/',
		$DATAROOT,
		$server,
		$node,
		DSC::extractor::yymmdd($when),
		$datafile . ".dat");
}

sub load_data {
	my $self = shift;
	my $datadir;
	my %hash;
	my $start_t = time;
	my $last = $self->{ARGS}->{end};
	my $first = $self->{ARGS}->{end} - $self->{ARGS}->{window};
	my $nl = 0;
	if ($self->{PLOT}->{plot_type} =~ /^accum/) {
		#
		# for 'accum' plots, round up the start time
		# to the beginning of the next 24hour period
		#
		$first += (86400 - ($self->{ARGS}->{end} % 86400));
	}
	if (($last - $first < 86400) && DSC::extractor::yymmdd($last) ne DSC::extractor::yymmdd($first)) {
		# check if first and last fall on different days (per $TZ)
		# and the window is less than a day.  If so, we need
		# to make sure that the datafile that 'last' lies in
		# gets read.
		$last = $first + 86400;
	}
	foreach my $node (@{$self->{CFG}->{servers}{$self->{ARGS}->{server}}}) {
	  next if ($self->{ARGS}->{node} ne 'all' && $node ne $self->{ARGS}->{node});
	  foreach my $real_node (@{$self->{CFG}->{nodemap}{$self->{ARGS}->{server}}{$node}}) {
	    for (my $t = $first; $t <= $last; $t += 86400) {
		my %thash;
		my $datafile = $self->datafile_path($self->{ARGS}->{plot}, $self->{ARGS}->{server}, $real_node, $t);
		$self->debug(1, "reading $datafile");
		#warn "$datafile: $!\n" unless (-f $datafile);
		if ('bynode' eq $self->{ARGS}->{plot}) {
			# XXX ugly special case
			$nl += &{$self->{PLOT}->{data_reader}}(\%thash, $datafile);
			$self->bynode_summer(\%thash, \%hash, $node);
			# XXX yes, wasteful repetition
			@{$self->{plotkeys}} = @{$self->{CFG}->{servers}{$self->{ARGS}->{server}}};
			@{$self->{plotnames}} = @{$self->{CFG}->{servers}{$self->{ARGS}->{server}}};
		} else {
			# note that 'summing' is important for 'accum'
			# plots, even for a single node
			$nl += &{$self->{PLOT}->{data_reader}}(\%thash, $datafile);
			my $start_t = time;
			$self->{PLOT}->{data_summer}(\%thash, \%hash);
			my $stop_t = time;
			$self->debugf(1, "data_summer took %d seconds", $stop_t-$start_t);
		}
	    }
	  }
	}
	my $stop_t = time;
	$self->debugf(1, "reading datafile took %d seconds, %d lines",
		$stop_t-$start_t,
		$nl);
	\%hash;
}

sub munge_data {
	my $self = shift;
	my $data = shift;
	if (defined($self->{PLOT}->{munge_func})) {
		$self->debug(1, "munging");
		$data = $self->{PLOT}->{munge_func}($self, $data);
	}
	if ($self->{ARGS}->{yaxis} && $self->{ARGS}->{yaxis} eq 'percent') {
		$self->debug(1, "converting to percentage");
		$data = $self->convert_to_percentage($data, $self->{PLOT}->{plot_type});
	}
	$data;
}

sub write_data {
	my $self = shift;
	my $datafh = shift;	# a file handle
	my $data = shift;
	switch ($self->{PLOT}->{plot_type}) {
	case 'trace'
	    { return $self->trace_data_to_tmpfile($data, $datafh); }
	case 'trace_line'
	    { return $self->trace_data_to_tmpfile($data, $datafh); }
	case 'accum1d'
	    { return $self->accum1d_data_to_tmpfile($data, $datafh); }
	case 'accum2d'
	    { return $self->accum2d_data_to_tmpfile($data, $datafh); }
	case 'hist2d' {
	    # hist2d is for qtype_vs_qnamelen
	    # assumes "bell-shaped" curve and cuts off x-axis at 5% and 95%
	    return $self->hist2d_data_to_tmpfile($data, $datafh);
	    }
	else
	    { $self->error("Unknown plot type: $self->{PLOT}->{plot_type}");}
	}
	return 0;
}

sub plot_data {
	my $self = shift;
	my $datafname = shift;
	my $cachefname = shift;
	switch ($self->{PLOT}->{plot_type}) {
	case 'trace'
	    { $self->trace_plot($datafname, $self->{ARGS}->{binsize}, $cachefname); }
	case 'trace_line'
	    { $self->trace_line_plot($datafname, $self->{ARGS}->{binsize}, $cachefname); }
	case 'accum1d'
	    { $self->accum1d_plot($datafname, $self->{ARGS}->{binsize}, $cachefname); }
	case 'accum2d'
	    { $self->accum2d_plot($datafname, $self->{ARGS}->{binsize}, $cachefname); }
	case 'hist2d' {
	    # like for qtype_vs_qnamelen
	    # assumes "bell-shaped" curve and cuts off x-axis at 5% and 95%
	    $self->hist2d_plot($datafname, $self->{ARGS}->{binsize}, $cachefname);
	    }
	else
	    { $self->error("Unknown plot type: $self->{PLOT}->{plot_type}"); }
	}
}

sub trace_data_to_tmpfile {
	my $self = shift;
	my $data = shift;
	my $tf = shift;
	my $start_t = time;
	my $nl;
	if (defined($self->{PLOT}->{data_dim}) && 1 == $self->{PLOT}->{data_dim}) {
	    $nl = Ploticus_create_datafile_keyless($data,
		$self->{plotkeys},
		$tf,
		$self->{ARGS}->{binsize},
		$self->{ARGS}->{end},
		$self->{ARGS}->{window},
		$self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{divideflag});
	} else {
	    $nl = Ploticus_create_datafile($data,
		$self->{plotkeys},
		$tf,
		$self->{ARGS}->{binsize},
		$self->{ARGS}->{end},
		$self->{ARGS}->{window},
		$self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{divideflag});
	}
	my $stop_t = time;
	$self->debugf(1, "writing trace tmpfile took %d seconds, %d lines",
		$stop_t-$start_t,
		$nl);
	$nl;
}

# calculate the amount of time in an 'accum' dataset.
#
sub calc_accum_win {
	my $self = shift;
	$self->debug(1, "calc_accum_win: ".Dumper($self->{ARGS}));
	my $last = $self->{ARGS}->{end};
	my $first = $self->{ARGS}->{end} - $self->{ARGS}->{window};
	$first += (86400 - ($self->{ARGS}->{end} % 86400));
	$self->debugf(1, "accum window = %.2f days", ($last - $first) / 86400);
	$last - $first;
}

sub accum1d_data_to_tmpfile {
	my $self = shift;
	my $data = shift;
	my $tf = shift;
	my $start_t = time;
	my $accum_win = $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{divideflag} ?
		$self->calc_accum_win() : 1;
	my $n;
	delete $data->{$DSC::extractor::SKIPPED_KEY};
	delete $data->{$DSC::extractor::SKIPPED_SUM_KEY};
	$n = 0;
	$self->debug(2, "accum1d_data_to_tmpfile: sorting $data...");
	foreach my $k1 (sort {$data->{$b} <=> $data->{$a}} keys %$data) {
		$self->debug(2, "accum1d_data_to_tmpfile: k1=$k1");
		$tf->print(join(' ',
			$k1,
			$data->{$k1} / $accum_win,
			&{$self->{PLOT}->{label_func}}($k1),
			&{$self->{PLOT}->{color_func}}($k1),
			), "\n");
		last if (++$n == $self->{ACCUM_TOP_N});
	}
	$self->debug(1, "accum1d_data_to_tmpfile: loop done...");
	close($tf);
	my $stop_t = time;
	$self->debugf(1, "writing accum tmpfile took %d seconds, %d lines",
		$stop_t-$start_t,
		$n);
	$n;
}

sub accum2d_data_to_tmpfile {
	my $self = shift;
	my $data = shift;
	my $tf = shift;
	my $start_t = time;
	my %accum_sum;
	my $accum_win = $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{divideflag} ?
		$self->calc_accum_win() : 1;
	my $n;
	delete $data->{$DSC::extractor::SKIPPED_KEY};
	delete $data->{$DSC::extractor::SKIPPED_SUM_KEY};
	foreach my $k1 (keys %$data) {
		foreach my $k2 (keys %{$data->{$k1}}) {
			next unless (grep {$k2 eq $_} @{$self->{plotkeys}});
			$accum_sum{$k1} += $data->{$k1}{$k2};
		}
	}
	$n = 0;
	foreach my $k1 (sort {$accum_sum{$b} <=> $accum_sum{$a}} keys %accum_sum) {
		my @vals;
		foreach my $k2 (@{$self->{plotkeys}}) {
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
		last if (++$n == $self->{ACCUM_TOP_N});
	}
	close($tf);
	my $stop_t = time;
	$self->debugf(1, "writing $n lines to tmpfile took %d seconds", $stop_t-$start_t);
	$n;
}

sub hist2d_data_to_tmpfile {
	my $self = shift;
	my $data = shift;
	my $tf = shift;
	my $start_t = time;
	delete $data->{$DSC::extractor::SKIPPED_KEY};
	delete $data->{$DSC::extractor::SKIPPED_SUM_KEY};
	
	# find 5th,95th percentile for auto x-axis scaling
	my $min_k2 = 1000000;
	my $max_k2 = 0;
	my $sum = 0;
	# first find min,max k2 range, and calc sum
	foreach my $k1 (@{$self->{plotkeys}}) {
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
		foreach my $k1 (@{$self->{plotkeys}}) {
			$sum2 += $data->{$k1}{$k2} || 0;
		}
		$p1 = $k2 if ($sum2 / $sum < 0.05);
		$p2 = $k2 if ($sum2 / $sum > 0.95);
		last if (defined($p1) && defined($p2));
	}
	
	my $nl = 0;
	foreach my $k2 ($p1..$p2) {
		my @vals;
		foreach my $k1 (@{$self->{plotkeys}}) {
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
	my $stop_t = time;
	$self->debugf(1, "writing $nl lines to tmpfile took %d seconds", $stop_t-$start_t);
	#system "cat $tf 1>&2";
	$nl;
}

sub time_descr {
	my $self = shift;
	my $from_t = $self->{ARGS}->{end} - $self->{ARGS}->{window};
	my $to_t = $self->{ARGS}->{end};
	if ($self->{PLOT}->{plot_type} =~ /^accum/) {
		$from_t += (86400 - ($self->{ARGS}->{end} % 86400));
		$to_t += (86400 - ($self->{ARGS}->{end} % 86400) - 1);
		$to_t = $self->{now} if ($to_t > $self->{now});
	}
	my $from_ctime = POSIX::strftime("%b %d, %Y, %T", localtime($from_t));
	my $to_ctime = POSIX::strftime("%b %d, %Y, %T %Z", localtime($to_t));
	"From $from_ctime To $to_ctime";
}

sub find_default_yaxis_type {
	my $self = shift;
	foreach my $t (keys %{$self->{PLOT}->{yaxes}}) {
		return $t if ($self->{PLOT}->{yaxes}{$t}{default});
	}
	#die "[$$] no default yaxis type for $self->{ARGS}->{plot}\n";
	'none';
}

sub trace_plot {
	my $self = shift;
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = $self->cache_image_path($cache_name);
	my $ntypes = int(@{$self->{plotnames}});
	my $start_t = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($self->{PLOT}->{map_legend}) {
		$mapfile = $self->cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_arg("-maxrows", "2000000");
	ploticus_arg("-maxfields", "2000000");
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	#system "cat $tf 1>&2";

	my $areadef_opts = {
		-title => $self->{PLOT}->{plottitle} . "\n" . $self->time_descr(),
		-rectangle => '1 1 6 4',
		-xscaletype => 'datetime mm/dd/yy',
		-ystackfields => join(',', 2..($ntypes+1)),
		-end => $self->{ARGS}->{end},
		-window => $self->{ARGS}->{window},
	};
	my $xaxis_opts = {
		-window => $self->{ARGS}->{window},
		-stubcull => '0.5',
	};
	my $yaxis_opts = {
		-label => $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{label},
		-grid => 'yes',
	};
	my $bars_opts = {
		-labelsarrayref => $self->{plotnames},
		-colorsarrayref => $self->{plotcolors},
		-indexesarrayref => [0..$ntypes-1],
		-keysarrayref => $self->{plotkeys},
		-barwidth => 4.5 / ($self->{ARGS}->{window} / $binsize),
	};
	if (defined($mapfile)) {
		my %copy = %{$self->{ARGS}};
		delete $copy{key};
		my $uri = $self->urlpath(%copy);
		$uri .= '&key=@KEY@';
		$self->debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}

	if ($self->{ARGS}->{mini}) {
		$areadef_opts->{-title} = $self->{PLOT}->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 4 2';
		delete $yaxis_opts->{-label};
		$xaxis_opts->{-mini} = 'yes';
	}

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($self->{ARGS}->{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop_t = time;
	$self->debugf(1, "ploticus took %d seconds", $stop_t-$start_t);
}

sub trace_line_plot {
	my $self = shift;
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = $self->cache_image_path($cache_name);
	my $ntypes = int(@{$self->{plotnames}});
	my $start_t = time;
	my $mapfile = undef;
	ploticus_init("png", "$pngfile.new");
	if ($self->{PLOT}->{map_legend}) {
		$mapfile = $self->cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_arg("-maxrows", "20000");
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	my $areadef_opts = {
		-title => $self->{PLOT}->{plottitle} . "\n" . $self->time_descr(),
		-rectangle => '1 1 6 4',
		-xscaletype => 'datetime mm/dd/yy',
		-ystackfields => join(',', 2..($ntypes+1)),
		-end => $self->{ARGS}->{end},
		-window => $self->{ARGS}->{window},
	};
	my $xaxis_opts = {
		-window => $self->{ARGS}->{window},
		-stubcull => '0.5',
	};
	my $yaxis_opts = {
		-label => $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{label},
		-grid => 'no',
	};
	my $lines_opts = {
		-labelsarrayref => $self->{plotnames},
		-colorsarrayref => $self->{plotcolors},
		-indexesarrayref => [0..$ntypes-1],
		-keysarrayref => $self->{plotkeys},
		-barwidth => 4.5 / ($self->{ARGS}->{window} / $binsize),
	};
	if (defined($mapfile)) {
		my %copy = %{$self->{ARGS}};
		delete $copy{key};
		my $uri = $self->urlpath(%copy);
		$uri .= '&key=@KEY@';
		$self->debug(1, "click URI = $uri");
		$lines_opts->{-legend_clickmapurl_tmpl} = $uri;
	}
	if ($self->{ARGS}->{mini}) {
		$areadef_opts->{-title} = $self->{PLOT}->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 4 2';
		delete $yaxis_opts->{-label};
		$xaxis_opts->{-mini} = 'yes';
	}
	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_lines($lines_opts);
	Ploticus_legend() unless ($self->{ARGS}->{mini});
	ploticus_end();
	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop_t = time;
	$self->debugf(1, "ploticus took %d seconds", $stop_t-$start_t);
}

sub accum1d_plot {
	my $self = shift;
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = $self->cache_image_path($cache_name);
	my $ntypes = int(@{$self->{plotnames}});
	my $start_t = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($self->{PLOT}->{map_legend}) {
		$mapfile = $self->cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	Ploticus_categories(1);
	my $areadef_opts = {
		-title => $self->{PLOT}->{plottitle} . "\n" . $self->time_descr(),
		-rectangle => '3 1 8 6',
		-yscaletype => 'categories',
		-xstackfields => '2',
	};
	my $xaxis_opts = {
		# yes we abuse/confuse x/y axes
		-label => $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{label},
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

	if ($self->{ARGS}->{mini}) {
		$areadef_opts->{-title} = $self->{PLOT}->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 3 4';
		delete($xaxis_opts->{-label});
	}

	if (defined($mapfile)) {
		my %copy = %{$self->{ARGS}};
		delete $copy{key};
		my $uri = $self->urlpath(%copy);
		$uri .= '&key=@KEY@';
		$self->debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}
 
	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	unless ($self->{ARGS}->{mini}) {
		for(my $i=0;$i<$ntypes;$i++) {
			Ploticus_legendentry({
				-label => $self->{plotnames}->[$i],
				-details => $self->{plotcolors}->[$i],
				-tag => $self->{plotkeys}->[$i],
			});
		}
	}
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($self->{ARGS}->{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop_t = time;
	$self->debugf(1, "ploticus took %d seconds", $stop_t-$start_t);
}

sub accum2d_plot {
	my $self = shift;
	my $tf = shift;
	my $binsize = shift;
	my $cache_name = shift;
	my $pngfile = $self->cache_image_path($cache_name);
	my $ntypes = int(@{$self->{plotnames}});
	my $start_t = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($self->{PLOT}->{map_legend}) {
		$mapfile = $self->cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	Ploticus_categories(1);
	my $areadef_opts = {
		-title => $self->{PLOT}->{plottitle} . "\n" . $self->time_descr(),
		-rectangle => '3 1 8 6',
		-yscaletype => 'categories',
		-xstackfields => join(',', 2..($ntypes+1)),
	};
	my $xaxis_opts = {
		# yes we abuse/confuse x/y axes
		-label => $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{label},
		-grid => 'yes',
	};
	my $yaxis_opts = {
		-stubs => 'usecategories',
	};
	my $bars_opts = {
		-labelsarrayref => $self->{plotnames},
		-colorsarrayref => $self->{plotcolors},
		-keysarrayref => $self->{plotkeys},
		-indexesarrayref => [0..$ntypes-1],
		-horizontalbars => 'yes',
	};
	my $legend_opts = {
		-reverseorder => 'no',
	};

	if ($self->{ARGS}->{mini}) {
		$areadef_opts->{-title} = $self->{PLOT}->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 3 4';
		delete($xaxis_opts->{-label});
	}

	if (defined($mapfile)) {
		my %copy = %{$self->{ARGS}};
		delete $copy{key};
		my $uri = $self->urlpath(%copy);
		$uri .= '&key=@KEY@';
		$self->debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}
 
	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($self->{ARGS}->{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop_t = time;
	$self->debugf(1, "ploticus took %d seconds", $stop_t-$start_t);
}

sub hist2d_plot {
	my $self = shift;
	my $tf = shift;
	my $binsize = shift;	# ignored
	my $cache_name = shift;
	my $pngfile = $self->cache_image_path($cache_name);
	my $ntypes = int(@{$self->{plotnames}});
	my $start_t = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	if ($self->{PLOT}->{map_legend}) {
		$mapfile = $self->cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	Ploticus_categories(1);
	my $areadef_opts = {
		-title => $self->{PLOT}->{plottitle} . "\n" . $self->time_descr(),
		-rectangle => '1 1 6 4',
		-ystackfields => join(',', 2..($ntypes+1)),
	};
	my $yaxis_opts = {
		-label => $self->{PLOT}->{yaxes}{$self->{ARGS}->{yaxis}}{label},
		-grid => 'yes',
	};
	my $xaxis_opts = {
		-label => $self->{PLOT}->{xaxislabel},
	};
	my $bars_opts = {
		-labelsarrayref => $self->{plotnames},
		-colorsarrayref => $self->{plotcolors},
		-keysarrayref => $self->{plotkeys},
		-indexesarrayref => [0..$ntypes-1],
	};
	my $legend_opts = {
		-reverseorder => 'no',
	};

	if ($self->{ARGS}->{mini}) {
		$areadef_opts->{-title} = $self->{PLOT}->{plottitle};
		$areadef_opts->{-rectangle} = '1 1 3 4';
		delete($xaxis_opts->{-label});
	}

	if (defined($mapfile)) {
		my %copy = %{$self->{ARGS}};
		delete $copy{key};
		my $uri = $self->urlpath(%copy);
		$uri .= '&key=@KEY@';
		$self->debug(1, "click URI = $uri");
		$bars_opts->{-legend_clickmapurl_tmpl} = $uri;
	}

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_bars($bars_opts);
	Ploticus_legend() unless ($self->{ARGS}->{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop_t = time;
	$self->debugf(1, "ploticus took %d seconds", $stop_t-$start_t);
}

sub error {
	my $self = shift;
	my $msg = shift;
	if ($self->cgi) {
		print $self->cgi->header(-type=>'text/html',-expires=>$expires_time);
		print "<h2>$0 ERROR</h2><p>$msg\n";
	} else {
		print STDERR "ERROR: $msg\n";
	}
	exit(1);
}

sub dumpdata {
	my $self = shift;
	my $ref = shift;
	print $self->cgi->header(-type=>'text/plain',-expires=>$expires_time);
	print Dumper($ref);
}

sub convert_to_percentage {
	my $self = shift;
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
	my $self = shift;
	my $ctx = Digest::MD5->new;
	$ctx->add(grep {defined($_)} @_);
	$ctx->hexdigest;
}

sub cache_image_path {
	my $self = shift;
	my $prefix = shift || die;
	"/usr/local/dsc/cache/$prefix.png";
}

sub cache_mapfile_path {
	my $self = shift;
	my $prefix = shift || confess "cache_mapfile_path: no prefix given";
	"/usr/local/dsc/cache/$prefix.map";
}

# return 0 if we should generate a cached image
# return 1 if the cached image is useable
sub check_image_cache {
	my $self = shift;
	my $prefix = shift;
	my $f = $self->cache_image_path($prefix);
	my @sb = stat($f);
	return 0 unless (@sb);
	unless (time - $sb[9] < $CacheImageTTL) {
		unlink $f;
		return 0;
	}
	return 1;
}

sub cached_image_size {
	my $self = shift;
	my $prefix = shift;
	my @sb = stat($self->cache_image_path($prefix));
	return -1 unless (@sb);
	$sb[7];
}

sub cat_image {
	my $self = shift;
	my $prefix = shift;
	if (open(F, $self->cache_image_path($prefix))) {
		print while (<F>);
		close(F);
	} else {
		#die "$prefix: $!\n";
	}
}

sub mapfile_to_buf {
	my $self = shift;
	my $prefix = shift || die;
	my $buf = '';
	if (open(F, $self->cache_mapfile_path($prefix))) {
		$buf .= $_ while (<F>);
		close(F);
	} else {
		#die "$prefix: $!\n";
	}
	$buf;
}

sub image_to_buf {
	my $self = shift;
	my $prefix = shift;
	my $buf = '';
	if (open(F, $self->cache_image_path($prefix))) {
		$buf .= $_ while (<F>);
		close(F);
	} else {
		#die "$prefix: $!\n";
	}
	$buf;
}

sub valid_tld_filter {
	my $self = shift;
	my $tld = shift;
	@{$self->{valid_domains}} = DSC::grapher::config::get_valid_domains($self->{ARGS}->{server})
		unless $self->{valid_domains};
	grep {$_ && $tld eq $_} @{$self->{valid_domains}};
}

sub numeric_tld_filter {
	my $self = shift;
	my $tld = shift;
	return ($tld =~ /^[0-9]+$/) ? 1 : 0;
}

sub invalid_tld_filter {
	my $self = shift;
	my $tld = shift;
	return 0 if ($self->valid_tld_filter($tld));
	return 0 if ($self->numeric_tld_filter($tld));
	return 1;
}

sub debug {
	my $self = shift;
	my $l = shift;
	return unless $dbg_lvl >= $l;
	return unless defined $self->{CFG}->{debug_fh};
	$self->{CFG}->{debug_fh}->print("[$$] ");
	$self->{CFG}->{debug_fh}->print(@_);
	$self->{CFG}->{debug_fh}->print("\n");
}

sub debugf {
	my $self = shift;
	my $l = shift;
	return unless ($dbg_lvl >= $l);
	return unless defined $self->{CFG}->{debug_fh};
	$self->{CFG}->{debug_fh}->print("[$$] ");
	$self->{CFG}->{debug_fh}->printf(@_);
	$self->{CFG}->{debug_fh}->print("\n");
}

#### MUNGERS ##############
#
# ARE class methods, but we use an ugly hack to call them
# as methods
#

sub munge_2d_to_1d {
	# this function changes a 2D array into a 1D array
	# by combining key values.   For example $dat{k1}{k2} becomes $dat{"k1:k2"}
	my $self = shift;
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

sub munge_sum_2d_to_1d {
	# this function changes a 2D array into a 1D array
	# by summing 2nd dim values.
	my $self = shift;
	my $data = shift;
	my %newdata;
	foreach my $t (keys %$data) {
		foreach my $k1 (keys %{$data->{$t}}) {
			foreach my $k2 (keys %{$data->{$t}{$k1}}) {
				$newdata{$t}{$k1} += $data->{$t}{$k1}{$k2};
			}
		}
	}
	\%newdata;
}

sub munge_anonymize_ip {
	# anonymize IP addresses, leaving only 1st octet.
	# since anonymizing may result in colissions, we sum the values
	my $self = shift;
	my $data = shift;
	return $data unless ($self->{CFG}->{anonymize_ip});
	my %newdata;
	foreach my $k1 (keys %$data) {
		next if ($k1 eq $DSC::extractor::SKIPPED_KEY);
		next if ($k1 eq $DSC::extractor::SKIPPED_SUM_KEY);
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

sub munge_min_max_mean {
	# Input data is 1D and must have numeric keys
	# Output is a hash with keys: min, max, mean
	my $self = shift;
	my $data = shift;
	my %newdata;
	my $j1;
	my $j2;
	my $N = 0;
	foreach my $t (keys %$data) {
		my $min = undef;
		my $max = undef;
		my $sum = 0;
		my $cnt = 0;
		foreach my $k1 (keys %{$data->{$t}}) {
			$min = $k1 if (!defined($min) || ($k1 < $min));
			$max = $k1 if (!defined($max) || ($k1 > $max));
			$sum += ($k1 * $data->{$t}{$k1});
			$cnt += $data->{$t}{$k1};
		}
		$cnt = 1 unless $cnt;
		$newdata{$t}{min} = $min;
		$newdata{$t}{max} = $min;
		$newdata{$t}{mean} = $sum / $cnt;
	}
	\%newdata;
}

##### DATA SUMMERS ###############
#
# are NOT class methods
#

sub data_summer_0d {
	my $from = shift;
	my $to = shift;
	foreach my $k0 (keys %$from) {
		$to->{$k0} += $from->{$k0};
	}
}

sub data_summer_1d {
	my $from = shift;
	my $to = shift;
	foreach my $k0 (keys %$from) {
		foreach my $k1 (keys %{$from->{$k0}}) {
			$to->{$k0}{$k1} += $from->{$k0}{$k1};
		}
	}
}

sub data_summer_2d {
	my $from = shift;
	my $to = shift;
	foreach my $k0 (keys %$from) {
		foreach my $k1 (keys %{$from->{$k0}}) {
			foreach my $k2 (keys %{$from->{$k0}{$k1}}) {
				$to->{$k0}{$k1}{$k2} += $from->{$k0}{$k1}{$k2};
			}
		}
	}
}

# XXX special hack for "bynode" plots
# XXX assume $from hash is 1D
# NOTE this is very similar to data_summer_1d
#
sub bynode_summer {
	my $self = shift;
	my $from = shift;
	my $to = shift;
	my $newkey = shift;
	foreach my $k0 (keys %$from) {
		foreach my $k1 (keys %{$from->{$k0}}) {
			$to->{$k0}{$newkey} += $from->{$k0}{$k1};
		}
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
	my $self = shift;
	my $cache_name = shift;
	my $imgsrc = '';
	my $result = '';

	my %attrs;

	if (my $reason = $self->reason_to_not_plot) {
		return "<div class=\"notice\">$reason</div>";
	}

	unless ($self->cached_image_size($cache_name) > 0) {
		return "<div class=\"notice\">No Data To Display At This Time</div>";
	}

	my $pn = $self->{ARGS}->{plot};
	my $withmap = ($DSC::grapher::plot::PLOTS{$pn}->{map_legend}) ? 1 : 0;
	if ($withmap) {
		$result .= $self->html_markup('map', {name=>'TheMap'}, $self->mapfile_to_buf($cache_name));
		$attrs{usemap} = '#TheMap';
	}
	
	if ($self->{use_data_uri}) {
		$imgsrc = "data:image/png;base64,\n";
		$imgsrc .= encode_base64($self->image_to_buf($cache_name));
	} else {
		my %own_args = %{$self->{ARGS}};	# copy
		while (my $k = shift) { $own_args{$k} = shift; }
		$own_args{content} = 'image';
		$imgsrc = $self->urlpath(%own_args);
	}
	$attrs{src} = $imgsrc;
	$result .= $self->html_markup('img', \%attrs, undef);
	$result;
}

sub urlpath {
	my $self = shift;
	my %args = @_;
	my $url = $ENV{REQUEST_URI};
	if ((my $n = index($url,'?')) > 0) {
		$url = substr($url,0,$n);
	}
	$self->delete_default_args(\%args);
	"$url?" . join('&', map {"$_=" . uri_escape($args{$_})} keys %args);
}

sub merge_args {
	my $self = shift;
	my %new = @_;
	my %old = %{$self->{ARGS}};
	foreach my $k (keys %new) {
		$old{$k} = $new{$k};
	}
	%old;
}

# NOTE NOT a class method
#
sub default_binsize {
	my $win = shift || (3600*4);
	my $bin = $win / 240;
	$bin = 60 if ($bin < 60);
	int($bin);
}

sub delete_default_args {
	my $self = shift;
	my $href = shift;
	my $pn = $self->{ARGS}->{plot};
	unless (defined($pn)) {
		carp "oops";
	}
	delete $href->{server} if ('none' eq $href->{server});
	delete $href->{node} if ('all' eq $href->{node});
	delete $href->{binsize} if (default_binsize($self->{ARGS}->{window}) == $href->{binsize});
	delete $href->{binsize} unless ($DSC::grapher::plot::PLOTS{$pn}->{plot_type} eq 'trace');
	delete $href->{end} if ((abs($self->{now} - $href->{end}) / $href->{window}) < 0.20);
	delete $href->{window} if (3600*4 == $href->{window});
	delete $href->{plot} if ('none' eq $href->{plot});
	delete $href->{content} if ('html' eq $href->{content});
	delete $href->{mini} if (0 == $href->{mini});
	delete $href->{yaxis} if ($self->find_default_yaxis_type() eq $href->{yaxis});
}

sub server { shift->{ARGS}->{server}; }
sub node { shift->{ARGS}->{node}; }

sub a_markup {
	my $self = shift;
	my $h = shift;
	my $a = shift;
	my $c = shift;
	my $t = shift;
	die "no h" unless (defined($h));
	die "no a" unless (defined($a));
	$self->html_markup('a',
		{ class => $c, href => $h, title => $t },
		$a);
}

sub img_markup {
	my $self = shift;
	my $s = shift;
	my $c = shift;
	my $a = shift;
	if ($self->{use_data_uri}) {
		$IconData{$s} = $self->load_icon_data($s) unless defined($IconData{$s});
		my $t;
		$t = "data:image/png;base64,\n";
		$t .= encode_base64($IconData{$s});
		$s = $t;
	} else {
		$s = "/dsc/$s";
	}
	$self->html_markup('img',
		{ class => $c, src => $s, alt => $a },
		undef);
}

sub navbar_item {
	my $self = shift;
	my $arg = shift;
	my $val = shift;
	my $label = shift;
	my $class = ($val eq $self->{ARGS}->{$arg}) ? 'current' : undef;
	my %newargs;
	$newargs{$arg} = $val;
	$newargs{node} = 'all' if ($arg eq 'server');
	$self->a_markup($self->urlpath($self->merge_args(%newargs)), $label, $class) . "\n";
}

sub navbar_arrow_item {
	my $self = shift;
	my $delta = shift;
	my $icon = shift;
	my $alt = shift;
	$self->a_markup($self->urlpath($self->merge_args(end=>$self->{ARGS}->{end} + $delta)),
		$self->img_markup("$icon.png", undef, $alt),
		undef,	# class
		$alt);	# title
}

sub navbar_servers_nodes {
	my $self = shift;
	my $snippet = '';
	$snippet .= "<ul>\n";
	my @items;
	foreach my $server ( @{$self->{CFG}->{serverlist}} ) {
		#print STDERR "server=$server\n";
		$snippet .= "<li>" . $self->navbar_item('server',$server,$server);
		next if $self->{CFG}->{'hide_nodes'};
		if ($self->{ARGS}->{server} eq $server) {
			foreach my $node (@{$self->{CFG}->{servers}{$server}}) {
				$snippet .= '<li>' . $sublist_item;
				$snippet .= $self->navbar_item('node',$node,$node);
			}
		}
	}
	$snippet .= "</ul>\n";
	$snippet;
}

sub navbar_plot {
	my $self = shift;
	my @items = ();
	my $pn = $self->{ARGS}->{plot} || die;
	push(@items, $self->navbar_item('plot','bynode','By Node')) if ($self->{ARGS}->{node} eq 'all');
	push(@items, $self->navbar_item('plot','qtype','Qtypes'));
	if ($pn eq 'qtype' || $pn eq 'dnssec_qtype') {
		push(@items, $sublist_item . $self->navbar_item('plot','dnssec_qtype','DNSSEC Qtypes'));
	}
	push(@items, $self->navbar_item('plot','rcode','Rcodes'));
	push(@items, $self->navbar_item('plot','client_subnet2_accum','Classification'));
	if ($pn =~ /^client_subnet2/) {
		push(@items, $sublist_item . $self->navbar_item('plot','client_subnet2_trace', 'trace'));
		push(@items, $sublist_item . $self->navbar_item('plot','client_subnet2_count', 'count'));
	}
	push(@items, $self->navbar_item('plot','client_subnet_accum','Client Geography'));
	push(@items, $self->navbar_item('plot','qtype_vs_all_tld','TLDs'));
	if ($pn =~ /qtype_vs_.*_tld/) {
		push(@items, $sublist_item . $self->navbar_item('plot','qtype_vs_valid_tld', 'valid'));
		push(@items, $sublist_item . $self->navbar_item('plot','qtype_vs_invalid_tld', 'invalid'));
		push(@items, $sublist_item . $self->navbar_item('plot','qtype_vs_numeric_tld', 'numeric'));
	}
	push(@items, $self->navbar_item('plot','second_ld_vs_rcode_accum','2nd Level Domains'));
	push(@items, $self->navbar_item('plot','third_ld_vs_rcode_accum','3rd Level Domains'));
	push(@items, $self->navbar_item('plot','client_addr_vs_rcode_accum','Rcodes by Client Address'));
	push(@items, $self->navbar_item('plot','certain_qnames_vs_qtype','Popular Names'));
	push(@items, $self->navbar_item('plot','ipv6_rsn_abusers_accum','IPv6 root abusers'));
	push(@items, $self->navbar_item('plot','opcode','Opcodes'));
	push(@items, $self->navbar_item('plot','query_attrs','Query Attributes'));
	if ($pn =~ /query_attrs|idn_qname|rd_bit|do_bit|edns_version|edns_bufsiz/) {
		push(@items, $sublist_item . $self->navbar_item('plot','idn_qname', 'IDN Qnames'));
		push(@items, $sublist_item . $self->navbar_item('plot','rd_bit', 'RD bit'));
		push(@items, $sublist_item . $self->navbar_item('plot','do_bit', 'DO bit'));
		push(@items, $sublist_item . $self->navbar_item('plot','qr_aa_bits','QR and AA bits'));
		push(@items, $sublist_item . $self->navbar_item('plot','edns_version', 'EDNS version'));
		push(@items, $sublist_item . $self->navbar_item('plot','edns_bufsiz', 'EDNS buffer size'));
	}
	push(@items, $self->navbar_item('plot','reply_attrs','Reply Attributes'));
	if ($pn =~ /reply_attrs|tc_bit/) {
		push(@items, $sublist_item . $self->navbar_item('plot','tc_bit', 'TC bit'));
	}
	push(@items, $self->navbar_item('plot','chaos_types_and_names','CHAOS'));
	push(@items, $self->navbar_item('plot','dns_ip_version','IP Version'));
	if ($pn =~ /dns_ip_version|dns_ip_version_vs_qtype/) {
	    push(@items, $sublist_item . $self->navbar_item('plot', 'dns_ip_version_vs_qtype', 'Query Types'));
	}
	push(@items, $self->navbar_item('plot','transport_vs_qtype','DNS Transport'));
	push(@items, $self->navbar_item('plot','direction_vs_ipproto','IP Protocols'));
	push(@items, $self->navbar_item('plot','qtype_vs_qnamelen','Qname Length'));
	push(@items, $self->navbar_item('plot','rcode_vs_replylen','Reply Lengths'));
	push(@items, $self->navbar_item('plot','client_port_range','Source Ports'));
	push(@items, $self->navbar_item('plot','priming_queries','Priming Queries'));
	push(@items, $self->navbar_item('plot','priming_responses','Priming Responses'));
	"<ul>\n" . join('<li>', '', @items) . "</ul>\n";
}

# This is function called from an "HTML" file by the template parser
#
sub navbar_window {
	my $self = shift;
	my @items = ();
	my $pn = $self->{ARGS}->{plot};
	if (defined($self->{PLOT}->{plot_type}) && $self->{PLOT}->{plot_type} =~ /^accum/) {
		foreach my $w (@{$self->{CFG}->{accum_windows}}) {
			push(@items, $self->navbar_item('window',$self->units_to_seconds($w),$w));
		}
		my @arrows;
		push(@arrows, $self->navbar_arrow_item(-$self->{ARGS}->{window}, '1leftarrow',
			"Backward " . $self->seconds_to_units($self->{ARGS}->{window})));
		push(@arrows, $self->navbar_arrow_item($self->{ARGS}->{window}, '1rightarrow',
			"Forward " . $self->seconds_to_units($self->{ARGS}->{window})));
		push(@items, $self->html_markup("div", {class=>'center'}, join("&nbsp;&nbsp;", @arrows)));
	} else {
		# trace
		foreach my $w (@{$self->{CFG}->{trace_windows}}) {
			push(@items, $self->navbar_item('window',$self->units_to_seconds($w),$w));
		}
		my @arrows;
		push(@arrows, $self->navbar_arrow_item(-$self->{ARGS}->{window}, '2leftarrow',
			"Backward " . $self->seconds_to_units($self->{ARGS}->{window})));
		push(@arrows, $self->navbar_arrow_item(-$self->{ARGS}->{window}/2, '1leftarrow',
			"Backward " . $self->seconds_to_units($self->{ARGS}->{window}/2)));
		push(@arrows, $self->navbar_arrow_item($self->{ARGS}->{window}/2, '1rightarrow',
			"Forward " . $self->seconds_to_units($self->{ARGS}->{window}/2)));
		push(@arrows, $self->navbar_arrow_item($self->{ARGS}->{window}, '2rightarrow',
			"Forward " . $self->seconds_to_units($self->{ARGS}->{window})));
		push(@items, $self->html_markup("div", {class=>'center'}, join("&nbsp;&nbsp;", @arrows)));
	}
	join('<br>', @items);
}


# This is function called from an "HTML" file by the template parser
#
sub navbar_yaxis {
	my $self = shift;
	my @items = ();
	my $pn = $self->{ARGS}->{plot};
	foreach my $t (keys %{$self->{PLOT}->{yaxes}}) {
		push(@items, $self->navbar_item('yaxis',$t, $self->{PLOT}->{yaxes}{$t}{label}));
	}
	join('<br>', @items);
}

sub units_to_seconds {
	my $self = shift;
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
	my $self = shift;
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
	my $self = shift;
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
	my $self = shift;
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
