package DSC::grapher::template;

use URI::Escape;

use strict;
use warnings;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
                &default_binsize
        );
        %EXPORT_TAGS = ( );
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

sub img_with_map {
	my $imgsrc;
	if (my $reason = DSC::grapher::reason_to_not_plot()) {
		return "<p>$reason";
	}
	
	if ($DSC::grapher::use_data_uri) {
		$imgsrc = "data:image/png;base64,\n";
		$imgsrc .= encode_base64(DSC::grapher::image_to_buf($DSC::grapher::cache_name));
	} else {
		my %own_args = %DSC::grapher::template::ARGS;	# copy
		while (my $k = shift) { $own_args{$k} = shift; }
		$own_args{content} = 'image';
		$imgsrc = urlpath(%own_args);
	}
	'<img src="' . $imgsrc . '">';
}

sub img {
	my %own_args = %DSC::grapher::template::ARGS;	# copy
	while (my $k = shift) { $own_args{$k} = shift; }
	$own_args{content} = 'image';
	my $p = urlpath(%own_args);
	my $q = "<img src=\"$p\">";
	#print STDERR "[$$] img() ret '$q'\n";
	$q;
}

sub page {
	my %own_args = %DSC::grapher::template::ARGS;	# copy
	while (my $k = shift) { $own_args{$k} = shift; }
	my $p = urlpath(%own_args);
	#print STDERR "[$$] page() ret '$p'\n";
	$p;
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
	my %old = %DSC::grapher::template::ARGS;
	foreach my $k (keys %new) {
		$old{$k} = $new{$k};
	}
	%old;
}

sub default_binsize {
	my $win = shift;
	my $bin = $win / 240;
	$bin = 60 if ($bin < 60);
	int($bin);
}

sub delete_default_args {
	my $href = shift;
	my $pn = $DSC::grapher::template::ARGS{plot};
	delete $href->{server} if ('none' eq $href->{server});
	delete $href->{node} if ('all' eq $href->{node});
	delete $href->{binsize} if (default_binsize($DSC::grapher::template::ARGS{window}) == $href->{binsize});
	delete $href->{binsize} unless ($DSC::grapher::config::PLOTS{$pn}->{plot_type} eq 'trace');
	delete $href->{end} if ((abs($DSC::grapher::template::now - $href->{end}) / $href->{window}) < 0.20);
	delete $href->{window} if (3600*4 == $href->{window});
	delete $href->{plot} if ('none' eq $href->{plot});
	delete $href->{content} if ('html' eq $href->{content});
	delete $href->{mini} if (0 == $href->{mini});
	delete $href->{yaxis} if (DSC::grapher::find_default_yaxis_type() eq $href->{yaxis});
}

sub server { $DSC::grapher::template::ARGS{server}; }
sub node { $DSC::grapher::template::ARGS{node}; }

sub a_markup {
	my $h = shift;
	my $a = shift;
	my $c = shift;
	die "no h" unless (defined($h));
	die "no a" unless (defined($a));
	"<a" . (defined($c) ? " class=\"$c\"" : '') . " href=\"$h\">$a</a>";
}

sub img_markup {
	my $s = shift;
	my $c = shift;
	"<img" . (defined($c) ? " class=\"$c\"" : '') . " src=\"$s\">";
}

sub navbar_item {
	my $arg = shift;
	my $val = shift;
	my $label = shift;
	my $class = ($val eq $DSC::grapher::template::ARGS{$arg}) ? 'current' : undef;
	my %newargs;
	$newargs{$arg} = $val;
	$newargs{node} = 'all' if ($arg eq 'server');
	a_markup(urlpath(merge_args(%newargs)), $label, $class) . "\n";
}

sub navbar_arrow_item {
	my $delta = shift;
	my $icon = shift;
	a_markup(urlpath(merge_args(end=>$DSC::grapher::template::ARGS{end} + $delta)), img_markup("/dsc-icons/$icon.png"));
}

sub sublist_item { '&rsaquo;&nbsp;'; }

sub navbar_servers_nodes {
	my $snippet = '';
	$snippet .= "<ul>\n";
	my @items;
	foreach my $server ( keys %{$DSC::grapher::template::CFG->{servers}} ) {
		#print STDERR "server=$server\n";
		$snippet .= "<li>" . navbar_item('server',$server,$server);
		if ($DSC::grapher::template::ARGS{server} eq $server) {
			foreach my $node (@{$DSC::grapher::template::CFG->{servers}{$server}}) {
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
	my $pn = $DSC::grapher::template::ARGS{plot} || die;
	push(@items, navbar_item('plot','bynode','By Node')) if ($DSC::grapher::template::ARGS{node} eq 'all');
	push(@items, navbar_item('plot','qtype','Qtypes'));
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
	push(@items, navbar_item('plot','certain_qnames_vs_qtype','Popular Names'));
	push(@items, navbar_item('plot','opcode','Opcodes'));
	push(@items, navbar_item('plot','query_attrs','Query Attributes'));
	if ($pn =~ /query_attrs|idn_qname|rd_bit|do_bit|edns_version/) {
		push(@items, sublist_item() . navbar_item('plot','idn_qname', 'IDN Qnames'));
		push(@items, sublist_item() . navbar_item('plot','rd_bit', 'RD bit'));
		push(@items, sublist_item() . navbar_item('plot','do_bit', 'DO bit'));
		push(@items, sublist_item() . navbar_item('plot','edns_version', 'EDNS version'));
	}
	push(@items, navbar_item('plot','direction_vs_ipproto','IP Protocols'));
	push(@items, navbar_item('plot','qtype_vs_qnamelen','Qname Length'));
	push(@items, navbar_item('plot','rcode_vs_replylen','Reply Lengths'));
	"<ul>\n" . join('<li>', '', @items) . "</ul>\n";
}

# This is function called from an "HTML" file by the template parser
#
sub navbar_window {
	my @items = ();
	my $pn = $DSC::grapher::template::ARGS{plot};
	my $PLOT = $DSC::grapher::config::PLOTS{$pn};
	if (defined($PLOT->{plot_type}) && $PLOT->{plot_type} =~ /^accum/) {
		foreach my $w (@{$DSC::grapher::template::CFG->{accum_windows}}) {
			push(@items, navbar_item('window',window_secs($w),$w));
		}
		my @arrows;
		push(@arrows, navbar_arrow_item(-$DSC::grapher::template::ARGS{window}, '1leftarrow'));
		push(@arrows, navbar_arrow_item($DSC::grapher::template::ARGS{window}, '1rightarrow'));
		push(@items, join("", @arrows));
	} else {
		# trace
		foreach my $w (@{$DSC::grapher::template::CFG->{trace_windows}}) {
			push(@items, navbar_item('window',window_secs($w),$w));
		}
		my @arrows;
		push(@arrows, navbar_arrow_item(-$DSC::grapher::template::ARGS{window}, '2leftarrow'));
		push(@arrows, navbar_arrow_item(-$DSC::grapher::template::ARGS{window}/2, '1leftarrow'));
		push(@arrows, navbar_arrow_item($DSC::grapher::template::ARGS{window}/2, '1rightarrow'));
		push(@arrows, navbar_arrow_item($DSC::grapher::template::ARGS{window}, '2rightarrow'));
		push(@items, join("", @arrows));
	}
	join('<br>', @items);
}


# This is function called from an "HTML" file by the template parser
#
sub navbar_yaxis {
	my @items = ();
	my $pn = $DSC::grapher::template::ARGS{plot};
	my $PLOT = $DSC::grapher::config::PLOTS{$pn};
	foreach my $t (keys %{$PLOT->{yaxes}}) {
		push(@items, navbar_item('yaxis',$t, $PLOT->{yaxes}{$t}{label}));
	}
	join('<br>', @items);
}

sub window_secs {
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
	} else {
		die "unknown unit: $unit";
	}
	$n;
}

1;
