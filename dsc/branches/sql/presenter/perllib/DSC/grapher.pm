package DSC::grapher;

use DSC::ploticus;
use DSC::extractor qw($SKIPPED_KEY $SKIPPED_SUM_KEY);
use DSC::db;
use DSC::grapher::plot;
use DSC::grapher::text;
use DSC::grapher::config;

use POSIX;
use List::Util qw(max);
use Getopt::Long;
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
	);
	%EXPORT_TAGS = ( );
	@EXPORT_OK   = qw();
}
use vars @EXPORT;
use vars @EXPORT_OK;

END { }

# constants
my $dbg_lvl = 0;
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
my %servers;
my $dbh;
my $base_uri;
my @valid_domains;
my %serverset;

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
	$base_uri = $ENV{REQUEST_URI};
	if ($base_uri) {
	    $base_uri =~ s/\?.*//; # remove parameters
	    $cgi = new CGI();
	    $use_data_uri = 0 if defined $cgi->param('x');
	}
	$now = time;
	@valid_domains = ();
	%serverset = ();
}

sub usage() {
    print STDERR <<EOF

Usage: $0 [options] fileprefix

Options:
  --server=servername (may be repeated)
  --node=nodename (may be repeated)
  --window=n (in seconds)
  --binsize=n (in seconds)
  --plot=plotname
  --mini
  --end=n (unix timestamp)
  --yaxis={rate,count,percent}
  --key=keyname
  --dbg_lvl=n

EOF
}

sub run {
	my $cfgfile = shift || '/usr/local/dsc/etc/dsc-grapher.cfg';
	my $cmdline = shift;
	# read config file early so we can set back the clock if necessary
	#
	$CFG = DSC::grapher::config::read_config($cfgfile);
	$now -= $CFG->{embargo} if defined $CFG->{embargo};

	if ($cgi) {
	    debug(1, "===> starting at " . POSIX::strftime('%+', localtime($now)));
	    debug(2, "Client is = " . ($ENV{REMOTE_ADDR}||'') . ":" .
		($ENV{REMOTE_PORT}||''));
	    # get HTTP parameters
	    my $untaint = CGI::Untaint->new($cgi->Vars);
	    $ARGS{server} = [$cgi->param('server')];
	    $ARGS{node} = [$cgi->param('node')];
	    $ARGS{window} = $untaint->extract(-as_integer => 'window');
	    $ARGS{binsize} = $untaint->extract(-as_integer => 'binsize');
	    $ARGS{plot} = $untaint->extract(-as_printable => 'plot');
	    $ARGS{content} = $untaint->extract(-as_printable => 'content');
	    $ARGS{mini} = $untaint->extract(-as_integer => 'mini');
	    $ARGS{end} = $untaint->extract(-as_integer => 'end');
	    $ARGS{yaxis} = $untaint->extract(-as_printable => 'yaxis') || undef;
	    $ARGS{key} = $untaint->extract(-as_printable => 'key');
	    $ARGS{nocache} = defined $cgi->param('nocache');
	    # http defaults
	    $ARGS{content} ||= 'html';
	} else {
	    # get command line arguments
	    $ARGS{dbg_lvl} = \$dbg_lvl;
	    if (!GetOptions(\%ARGS,
		"server=s@",
		"node=s@",
		"window=i",
		"binsize=i",
		"plot=s",
		"mini",
		"end=i",
		"yaxis=s",
		"key=s",
		"dbg_lvl=i",
		) || ($#ARGV != 0))
	    {
		usage();
		exit(1);
	    }
	    $ARGS{output} = $ARGV[0];
	}

	debug(3, 'CFG=' . Dumper($CFG)) if ($dbg_lvl >= 3);
	debug(3, "ENV=" . Dumper(\%ENV)) if ($dbg_lvl >= 3);
	}

	# common defaults
	$ARGS{server} ||= [()];
	$ARGS{node} ||= [()];
	$ARGS{window} ||= 3600*4;
	$ARGS{binsize} ||= default_binsize($ARGS{window});
	$ARGS{plot} ||= 'bynode';
	$ARGS{end} ||= $now;

	$PLOT = $DSC::grapher::plot::PLOTS{$ARGS{plot}};
	$TEXT = $DSC::grapher::text::TEXTS{$ARGS{plot}};
	error("Unknown plot type: $ARGS{plot}")
	    unless (defined ($PLOT));
	error("Time window cannot be larger than a month") if ($ARGS{window} > 86400*31);
	$dbg_lvl = $PLOT->{debugflag} if defined($PLOT->{debugflag});
	debug(3, "PLOT=" . Dumper($PLOT)) if ($dbg_lvl >= 3);

	$dbh = get_dbh;
	load_servers();

	# Sanity checking on CGI args

	for my $server (@{$ARGS{server}}) {
	    error("Unknown server: $server")
		unless $servers{$server}
	}
	for my $sn (@{$ARGS{node}}) {
	    my ($server, $node) = split('/', $sn);
	    error("Unknown node: $sn")
		unless $servers{$server}->{nodes}->{$node};
	}

	if (!defined($ARGS{yaxis})) {
		$ARGS{yaxis} = find_default_yaxis_type();
	} elsif (!defined($PLOT->{yaxes}{$ARGS{yaxis}})) {
		$ARGS{yaxis} = find_default_yaxis_type();
	}

	if ($PLOT->{plot_type} =~ /^accum|^hist/ && $ARGS{window} % 86400 > 0) {
		# round window up to multiple of a day
		$ARGS{window} += 86400 - $ARGS{window} % 86400;
	}

	$ARGS{end} = $now if ($ARGS{end} > $now);

	@plotkeys = ();
	@plotnames = ();
	@plotcolors = ();

	if ('bynode' eq $ARGS{plot}) {
	    # special case
	    for my $server (keys %servers) {
		my $allnodes = (grep $server eq $_, @{$ARGS{server}});
		for my $node (keys %{$servers{$server}->{nodes}}) {
		    my $name = $server . '/' . $node;
		    if ($allnodes || grep $name eq $_, @{$ARGS{node}}) {
			push @plotkeys, $servers{$server}->{nodes}->{$node};
			push @plotnames, $server . '/' . $node;
			my $i = $servers{$server}->{nodes}->{$node} - 1;
			$i %= $#{$PLOT->{colors}} + 1;
			push @plotcolors, ${$PLOT->{colors}}[$i];
		    }
		}
	    }
	} elsif ('byserver' eq $ARGS{plot}) {
	    # special case
	    for my $server (keys %servers) {
		if (grep($_ eq $server, @{$ARGS{server}}) ||
		    grep($_ =~ m!^$server/!, @{$ARGS{node}}))
		{
		    push @plotkeys, $servers{$server}->{id};
		    push @plotnames, $server;
		    my $i = $servers{$server}->{id} - 1;
		    $i %= $#{$PLOT->{colors}} + 1;
		    push @plotcolors, ${$PLOT->{colors}}[$i];
		}
	    }
	} elsif (defined($ARGS{key}) && grep {$_ eq $ARGS{key}} @{$PLOT->{keys}}) {
	    for (my $i = 0; $i<int(@{$PLOT->{keys}}); $i++) {
		next unless ($ARGS{key} eq ${$PLOT->{keys}}[$i]);
		push(@plotkeys, ${$PLOT->{keys}}[$i]);
		push(@plotnames, ${$PLOT->{names}}[$i]);
		push(@plotcolors, ${$PLOT->{colors}}[$i]);
	    }
	} else {
	    @plotkeys = @{$PLOT->{keys}};
	    @plotnames = @{$PLOT->{names}};
	    @plotcolors = @{$PLOT->{colors}};
	    delete $ARGS{key};
	}

	debug(1, "ARGS=" . Dumper(\%ARGS));
	$ACCUM_TOP_N = 20 if ($ARGS{mini});

	if ($cgi) {
	    my $cache_name = cache_name(
		    join(' ', @{$ARGS{server}}),
		    join(' ', @{$ARGS{node}}),
		    $ARGS{plot} . ($CFG->{anonymize_ip} ? '_anon' : ''),
		    $ARGS{end},
		    $ARGS{window},
		    $ARGS{binsize},
		    $ARGS{mini},
		    $ARGS{yaxis},
		    $ARGS{key});
	    debug(1, "cache_name = %s", $cache_name);
	    if ('html' eq $ARGS{content}) {
		if (!reason_to_not_plot()) {
			debug(1, "no reason to not plot");
			if ($ARGS{nocache} || !check_image_cache($cache_name)) {
				debug(1, "need to make cached image");
				make_image($cache_name);
			}
		}
		my $source = "/usr/local/dsc/share/html/plot.page";
		my $t = Text::Template->new(
			TYPE => 'FILE',
			SOURCE => $source,
			DELIMITERS => ['[[', ']]']
		);
		error("Text::Template failed for plot.page") unless defined ($t);
		print $cgi->header(-type=>'text/html',-expires=>$expires_time)
			unless (defined($CFG->{'no_http_header'}));
		my %vars_to_pass = (
			action => $base_uri,
			init_plottypes => init_plottypes(),
			navbar_servers_nodes => navbar_servers_nodes(),
			navbar_plot => navbar_plot(),
			navbar_window => navbar_window(),
			navbar_arrows => navbar_arrows(),
			navbar_yaxis => navbar_yaxis(),
			img_with_map => img_with_map($cache_name),
			description => $TEXT->{description},
			title => "DSC: " . ($PLOT->{plottitle}) . ": " .
			    join(", ", map(CGI::escapeHTML($_), @{$ARGS{server}}, @{$ARGS{node}})),
		);
		print $t->fill_in(
			PACKAGE => 'DSC::grapher::template',
			HASH => \%vars_to_pass,
			);
	    } else {
		make_image($cache_name) unless (reason_to_not_plot() || check_image_cache($cache_name));
		if (-f cache_image_path($cache_name)) {
			print $cgi->header(-type=>'image/png',-expires=>$expires_time)
			    unless (defined($CFG->{'no_http_header'}));
			cat_image($cache_name);
		}
	    }

	} else {
	    if (my $reason = reason_to_not_plot()) {
		error($reason);
	    } else {
		make_image($ARGS{output});
		unless (cached_image_size($ARGS{output}) > 0) {
		    error("No data to display at this time");
		}
	    }
	}

	debug(1, "<=== finished at " . POSIX::strftime('%+', localtime($now)));
}

sub reason_to_not_plot {
	return 'Please select one or more servers or nodes.'
	    if (!@{$ARGS{server}} && !@{$ARGS{node}});
	return 'Please select a plot.'
	    if ($ARGS{plot} eq 'none');
	return 'Please select (nodes of) a single server.'
	    if ($ARGS{plot} =~ /valid_tld/ && scalar(keys %serverset) > 1);
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
	debug(1, "Plotting $PLOT->{plottitle}: " .
	    join(',', @{$ARGS{server}}, @{$ARGS{node}}) .
	    " $ARGS{plot} $ARGS{end} $ARGS{window} $ARGS{binsize}");
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
		# assumes "bell-shaped" curve and cuts off x-axis at 5% and 95%
		hist2d_data_to_tmpfile($data, $datafile) and
		hist2d_plot($datafile, $ARGS{binsize}, $cache_name);
	} elsif ($PLOT->{plot_type} eq 'srcport') {
		# for plotting client source ports
		srcport_data_to_tmpfile($data, $datafile) and
		srcport_plot($datafile, $ARGS{binsize}, $cache_name);
	} else {
		error("Unknown plot type: $PLOT->{plot_type}");
	}
	$stop = time;
	debug(1, "graph took %d seconds", $stop-$start);
}

sub datafile_name {
	my $plot = shift;
	return $PLOT->{datafile} || $plot;
}

sub load_servers {
    # get server and node ids
    my $sth = $dbh->prepare(
	"SELECT server.name, server.server_id, node.name, node.node_id " .
	"FROM node JOIN server ON server.server_id = node.server_id");
    $sth->execute();
    while (my @row = $sth->fetchrow_array) {
	my ($server, $server_id, $node, $node_id) = @row;
	$servers{$server}->{id} = $server_id;
	$servers{$server}->{nodes}->{$node} = $node_id;
    }
    debug(2, 'servers=' . Dumper(%servers)) if ($dbg_lvl >= 2);
}

sub load_data {
	my %hash;
	my $start = time;
	my $last = $ARGS{end};
	$last += $ARGS{binsize} if ($PLOT->{plot_type} eq 'trace');
	my $first = $ARGS{end} - $ARGS{window};
	my $nl = 0;
	my $datafile = datafile_name($ARGS{plot});

	my $server_ids = [];
	my $node_ids = [];

	# get ids of requested servers and nodes
	for my $server (@{$ARGS{server}}) {
	    push @$server_ids, $servers{$server}->{id};
	    $serverset{$server} = 1;
	}
	for my $sn (@{$ARGS{node}}) {
	    my ($server, $node) = split('/', $sn);
	    debug(3, "server=$server, node=$node");
	    push @$node_ids, $servers{$server}->{nodes}->{$node};
	    $serverset{$server} = 1;
	}

	# load data
	debug(1, "reading $datafile");
	$nl += DSC::db::read_data($dbh, \%hash, $datafile,
	    $server_ids, $node_ids,
	    $first, $last, $PLOT->{nogroup}, $PLOT->{dbkeys}, $PLOT->{where});

	my $stop = time;
	debug(1, "reading datafile took %d seconds, %d lines",
		$stop-$start,
		$nl);
	return \%hash;
}

sub trace_data_to_tmpfile {
	my $data = shift;
	my $tf = shift;
	my $start = time;
	my $nl;
	if (defined($PLOT->{data_dim}) && 1 == $PLOT->{data_dim}) {
	    $nl = Ploticus_create_datafile_keyless($data,
		\@plotkeys,
		$tf,
		$ARGS{binsize},
		$ARGS{end},
		$ARGS{window},
		$PLOT->{yaxes}{$ARGS{yaxis}}{divideflag});
	} else {
	    $nl = Ploticus_create_datafile($data,
		\@plotkeys,
		$tf,
		$ARGS{binsize},
		$ARGS{end},
		$ARGS{window},
		$PLOT->{yaxes}{$ARGS{yaxis}}{divideflag});
	}
	my $stop = time;
	debug(1, "writing trace tmpfile took %d seconds, %d lines",
		$stop-$start,
		$nl);
	$nl;
}

# calculate the amount of time in an 'accum' dataset.
#
sub calc_accum_win {
	my $last = $ARGS{end};
	my $first = $ARGS{end} - $ARGS{window};
	if (my $remainder = $first % 86400) {
	    $first += (86400 - $remainder);
	}
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
	debug(2, "accum1d_data_to_tmpfile: sorting $data...");
	foreach my $k1 (sort {$data->{$b} <=> $data->{$a}} keys %$data) {
		debug(2, "accum1d_data_to_tmpfile: k1=$k1");
		$tf->print(join(' ',
			$k1,
			$data->{$k1} / $accum_win,
			&{$PLOT->{label_func}}($k1),
			&{$PLOT->{color_func}}($k1),
			), "\n");
		last if (++$n == $ACCUM_TOP_N);
	}
	debug(1, "accum1d_data_to_tmpfile: loop done...");
	close($tf);
	my $stop = time;
	debug(1, "writing accum tmpfile took %d seconds, %d lines",
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

sub srcport_data_to_tmpfile {
	my $data = shift;
	my $tf = shift;
	my $start = time;
	delete $data->{$SKIPPED_KEY};
	delete $data->{$SKIPPED_SUM_KEY};
	
	my $nl = 0;
	foreach my $k1 (sort {$data->{$b} <=> $data->{$a}} keys %$data) {
		print $tf "$nl $data->{$k1}\n";
		$nl++;
	}
	close($tf);
	my $stop = time;
	debug(1, "writing $nl lines to tmpfile took %d seconds", $stop-$start);
	#system "head $tf 1>&2";
	$nl;
}

sub time_descr {
	my $from_t = $ARGS{end} - $ARGS{window};
	my $to_t = $ARGS{end};
	if ($PLOT->{plot_type} =~ /^accum|^hist/) {
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
	if ($cgi && $PLOT->{map_legend}) {
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
		debug(1, "click URI = %s", $uri);
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
	if ($cgi && $PLOT->{map_legend}) {
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
		debug(1, "click URI = %s", $uri);
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
	if ($cgi && $PLOT->{map_legend}) {
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
		debug(1, "click URI = %s", $uri);
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
	if ($cgi && $PLOT->{map_legend}) {
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
		debug(1, "click URI = %s", $uri);
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

sub srcport_plot {
	my $tf = shift;
	my $binsize = shift;	# ignored
	my $cache_name = shift;
	my $pngfile = cache_image_path($cache_name);
	my $ntypes = @plotnames;
	my $start = time;
	my $mapfile = undef;

	ploticus_init("png", "$pngfile.new");
	ploticus_arg("-maxrows", "50000");
	if ($PLOT->{map_legend}) {
		$mapfile = cache_mapfile_path($cache_name);
		ploticus_arg("-csmap", "");
		ploticus_arg("-mapfile", "$mapfile.new");
	}
	ploticus_begin();
	Ploticus_getdata($tf->filename());
	my $areadef_opts = {
		-title => $PLOT->{plottitle} . "\n" . time_descr(),
		-rectangle => '1 1 6 4',
		-yfields => join(',', 2..($ntypes+1)),
		-xscaletype => 'log+1',
	};
	my $yaxis_opts = {
		-label => $PLOT->{yaxes}{$ARGS{yaxis}}{label},
		-grid => 'yes',
	};
	my $xaxis_opts = {
		-label => $PLOT->{xaxislabel},
	};
	my $lines_opts = {
		-labelsarrayref => \@plotnames,
		-colorsarrayref => \@plotcolors,
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
		$lines_opts->{-legend_clickmapurl_tmpl} = $uri;
	}

	Ploticus_areadef($areadef_opts);
	Ploticus_xaxis($xaxis_opts);
	Ploticus_yaxis($yaxis_opts);
	Ploticus_lines($lines_opts);
	#Ploticus_legend() unless ($ARGS{mini});
	ploticus_end();

	rename("$pngfile.new", $pngfile);
	rename("$mapfile.new", $mapfile) if defined($mapfile);
	my $stop = time;
	debug(1, "ploticus took %d seconds", $stop-$start);
}

sub error {
	my $msg = shift;
	if ($cgi) {
	    print $cgi->header(-type=>'text/html',-expires=>$expires_time);
	    print "<h2>$0 ERROR</h2><p>" . CGI::escapeHTML($msg) . "\n";
	} else {
	    print STDERR "$0 ERROR: " . $msg . "\n";
	}
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
	($cgi ? "/usr/local/dsc/cache/" : "") . "$prefix.png";
}

sub cache_mapfile_path {
	my $prefix = shift || die;
	($cgi ? "/usr/local/dsc/cache/" : "") . "$prefix.map";
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

sub valid_tld_filter {
	my $tld = shift;
	@valid_domains = DSC::grapher::config::get_valid_domains(
	    (keys %serverset)[0])
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
		return "<div class=\"notice\">No data to display at this time</div>";
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
	delete_default_args(\%args);
	my @newargs;
	while (my ($key, $value) = each %args) {
	    next unless $value;
	    if (ref $value) {
		# reference to a list of values
		push @newargs, map {"$key=" . uri_escape($_)} @$value;
	    } else {
		# single value
		push @newargs, "$key=" . uri_escape($value);
	    }
	}

	"$base_uri?" . join('&', @newargs);
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
	delete $href->{binsize} if (default_binsize($ARGS{window}) == $href->{binsize});
	delete $href->{binsize} unless ($DSC::grapher::plot::PLOTS{$pn}->{plot_type} eq 'trace');
	delete $href->{end} if ((abs($now - $href->{end}) / $href->{window}) < 0.20);
	delete $href->{window} if (3600*4 == $href->{window});
	delete $href->{plot} if ('none' eq $href->{plot});
	delete $href->{content} if ('html' eq $href->{content});
	delete $href->{mini} if (!$href->{mini});
	delete $href->{yaxis} if (find_default_yaxis_type() eq $href->{yaxis});
}

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
	a_markup(urlpath(merge_args(%newargs)), $label, $class) . "\n";
}

sub navbar_checkbox {
	my $arg = shift;
	my $val = shift;
	my $label = shift;
	my $attrs = { type => 'checkbox', name => $arg, value => $val };
	$attrs->{checked} = 'checked' if (grep $val eq $_, @{$ARGS{$arg}});
	html_markup('input', $attrs, undef) . $label;
}

sub navbar_option {
	my $arg = shift;
	my $val = shift;
	my $label = shift;
	my $attrs = shift || {};
	$attrs->{value} = $val;
	$attrs->{selected} = 'selected' if ($val eq $ARGS{$arg});
	html_markup('option', $attrs, $label) . "\n";
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
    my $buf = '';
    $buf .= "<ul>\n";
    my @items;
    foreach my $server ( keys %{$CFG->{servers}} ) {
	#print STDERR "server=$server\n";
	$buf .= "<li><div>" . navbar_checkbox('server',$server,$server) . "</div>\n";
	$buf .= "<ul>\n";
	foreach my $node (@{$CFG->{servers}{$server}}) {
	    $buf .= html_markup('li', undef, navbar_checkbox('node',"$server/$node",$node)) . "\n";
	}
	$buf .= "</ul></li>\n";
    }
    $buf .= "</ul>\n";
    $buf;
}

sub navbar_plot_option($) {
    my $val = shift;
    my $label = shift;

    my $PLOT = $DSC::grapher::plot::PLOTS{$val};
    navbar_option('plot', $val, $PLOT->{menutitle});
}

sub navbar_plot {
<<<<<<< .working
    my @items = ();
    push(@items, navbar_plot_option('byserver'));
    push(@items, navbar_plot_option('bynode'));
    #push(@items, '<optgroup label="Qtypes">');
	push(@items, navbar_plot_option('qtype'));
	push(@items, navbar_plot_option('dnssec_qtype'));
    #push(@items, '</optgroup>');
    push(@items, navbar_plot_option('rcode'));
    push(@items, navbar_plot_option('client_subnet2_accum'));
	push(@items, navbar_plot_option('client_subnet2_trace'));
	push(@items, navbar_plot_option('client_subnet2_count'));
    push(@items, navbar_plot_option('client_subnet_accum'));
    #push(@items, '<optgroup label="TLDs">');
	push(@items, navbar_plot_option('qtype_vs_all_tld'));
	push(@items, navbar_plot_option('qtype_vs_valid_tld'));
	push(@items, navbar_plot_option('qtype_vs_invalid_tld'));
	push(@items, navbar_plot_option('qtype_vs_numeric_tld'));
    #push(@items, '</optgroup>');
    push(@items, navbar_plot_option('client_addr_vs_rcode_accum'));
    push(@items, navbar_plot_option('certain_qnames_vs_qtype'));
    push(@items, navbar_plot_option('ipv6_rsn_abusers_accum'));
    push(@items, navbar_plot_option('opcode'));
    #push(@items, '<optgroup label="Query Attributes">');
	push(@items, navbar_plot_option('idn_qname'));
	push(@items, navbar_plot_option('rd_bit'));
	push(@items, navbar_plot_option('do_bit'));
	push(@items, navbar_plot_option('edns_version'));
    #push(@items, '</optgroup>');
    push(@items, navbar_plot_option('chaos_types_and_names'));
    push(@items, navbar_plot_option('direction_vs_ipproto'));
    push(@items, navbar_plot_option('qtype_vs_qnamelen'));
    push(@items, navbar_plot_option('rcode_vs_replylen'));
    join('', @items);
=======
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
	if ($pn =~ /query_attrs|idn_qname|rd_bit|do_bit|edns_version|edns_bufsiz/) {
		push(@items, sublist_item() . navbar_item('plot','idn_qname', 'IDN Qnames'));
		push(@items, sublist_item() . navbar_item('plot','rd_bit', 'RD bit'));
		push(@items, sublist_item() . navbar_item('plot','do_bit', 'DO bit'));
		push(@items, sublist_item() . navbar_item('plot','edns_version', 'EDNS version'));
		push(@items, sublist_item() . navbar_item('plot','edns_bufsiz', 'EDNS buffer size'));
	}
	push(@items, navbar_item('plot','chaos_types_and_names','CHAOS'));
	push(@items, navbar_item('plot','dns_ip_version','IP Version'));
	if ($pn =~ /dns_ip_version|dns_ip_version_vs_qtype/) {
	    push(@items, sublist_item() . navbar_item('plot', 'dns_ip_version_vs_qtype', 'Query Types'));
	}
	push(@items, navbar_item('plot','transport_vs_qtype','DNS Transport'));
	push(@items, navbar_item('plot','direction_vs_ipproto','IP Protocols'));
	push(@items, navbar_item('plot','qtype_vs_qnamelen','Qname Length'));
	push(@items, navbar_item('plot','rcode_vs_replylen','Reply Lengths'));
	push(@items, navbar_item('plot','client_port_range','Source Ports'));
	"<ul>\n" . join('<li>', '', @items) . "</ul>\n";
>>>>>>> .merge-right.r10282
}

# This is function called from an "HTML" file by the template parser
#
sub navbar_window {
    my @accum_windows = ();
    my @trace_windows = ();
    my @windows = ();
    my $accum_class = 'accum1d accum2d hist2d';
    my $trace_class = 'trace';
    my $pn = $ARGS{plot};
    my $PLOT = $DSC::grapher::plot::PLOTS{$pn};
    my $w;

    # merge trace_windows and accum_windows
    foreach my $w (@{$CFG->{trace_windows}}) {
	push(@trace_windows, [units_to_seconds($w), $w, $trace_class]);
    }
    foreach my $w (@{$CFG->{accum_windows}}) {
	push(@accum_windows, [units_to_seconds($w), $w, $accum_class]);
    }
    while (@trace_windows && @accum_windows) {
	if ($trace_windows[0][0] == $accum_windows[0][0]) {
	    shift @trace_windows;
	    $w = shift @accum_windows;
	    $w->[2] = $trace_class.' '.$accum_class;
	} elsif ($trace_windows[0][0] < $accum_windows[0][0]) {
	    $w = shift @trace_windows;
	} else {
	    $w = shift @accum_windows;
	}
	push @windows, $w;
    }
    push @windows, @trace_windows;
    push @windows, @accum_windows;

    my $buf = '';
    foreach my $w (@windows) {
	$buf .= navbar_option('window', $w->[0], $w->[1], {class=>$w->[2]});
    }

    $buf .= html_markup('input', {type=>'hidden', name=>'end', value=>$ARGS{end}}, undef);
    if ($ARGS{nocache}) {
	$buf .= html_markup('input', {type=>'hidden', name=>'nocache'}, undef);
    }
    if ($ARGS{key}) {
	$buf .= html_markup('input', {type=>'hidden', name=>'key', value=>$ARGS{key}}, undef);
    }
    $buf;
}

# This is function called from an "HTML" file by the template parser
#
sub navbar_arrows {
    my @arrows = ();

#    navbar_option('end', $ARGS{end}+$delta, 
#	"Backward " . seconds_to_units($ARGS{window})));

    push(@arrows, navbar_arrow_item(-$ARGS{window}, '2leftarrow',
	"Backward " . seconds_to_units($ARGS{window})));
    push(@arrows, navbar_arrow_item(-$ARGS{window}/2, '1leftarrow',
	"Backward " . seconds_to_units($ARGS{window}/2)));
    push(@arrows, navbar_arrow_item($ARGS{window}/2, '1rightarrow',
	"Forward " . seconds_to_units($ARGS{window}/2)));
    push(@arrows, navbar_arrow_item($ARGS{window}, '2rightarrow',
	"Forward " . seconds_to_units($ARGS{window})));

    join("&nbsp;&nbsp;", @arrows);
}


# This is function called from an "HTML" file by the template parser
#
sub init_plottypes {
    my $buf = "var PLOTS = {\n";
    while (my ($pn, $PLOT) = (each %DSC::grapher::plot::PLOTS)) {
	next if $PLOT->{plot_type} eq 'none';
	$buf .= "  $pn: {\n";
	$buf .= "    plot_type:  \"" . $PLOT->{plot_type} . "\",\n";
	$buf .= "    axes: { ";
	for my $t (qw(rate count percent)) {
	    $buf .= "$t: ";
	    if ($PLOT->{yaxes}{$t}{label}) {
		$buf .= '"' . $PLOT->{yaxes}{$t}{label} . '"';
	    } else {
		$buf .= 'null';
	    }
	    $buf .= ", ";
	}
	$buf .= "},\n";
	$buf .= "  },\n";
    }
    $buf .= "};\n";
    $buf;
}

# This is function called from an "HTML" file by the template parser
#
sub navbar_yaxis {
    my @items = ();
    my $pn = $ARGS{plot};
    my $PLOT = $DSC::grapher::plot::PLOTS{$pn};
    for my $t (qw(rate count percent)) {
	if ($PLOT->{yaxes}{$t}{label}) {
	    push(@items, navbar_option('yaxis', $t, $PLOT->{yaxes}{$t}{label}));
	} else {
	    push(@items, navbar_option('yaxis', $t, $t,
		{style=>'display:none;', disabled=>'disabled'}));
	}
    }
    join('', @items);
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
