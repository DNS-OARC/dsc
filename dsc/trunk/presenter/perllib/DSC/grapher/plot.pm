package DSC::grapher::config;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
                %PLOTS
        );
        %EXPORT_TAGS = ( );
        @EXPORT_OK   = qw();
	print STDERR "this is DSC::grapher::config::BEGIN\n";
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { 
	print STDERR "this is DSC::grapher::config::END\n";
}

use strict;
use warnings;
use NDBM_File;

my $qtype_keys	= [ qw(1 2  5     6   12  15 28   33  38 255 else) ];
my $qtype_names	= [ qw(A NS CNAME SOA PTR MX AAAA SRV A6 ANY Other) ];
my $qtype_colors = [ qw(red orange yellow brightgreen brightblue purple magenta redorange yellow2 green darkblue) ];

my $client_subnet2_keys =   [ qw(
	ok
	non-auth-tld
	root-servers.net
	localhost
	a-for-a
	a-for-root
	rfc1918-ptr
	funny-qclass
	funny-qtype
	src-port-zero
	malformed
	) ];

my $client_subnet2_names = [
        "Other",
        "Non-Authoritative TLD",
        "root-servers.net",
        "localhost",
        "A-for-A",
        "A-for-.",
        "RFC1918 PTR",
        "Funny Qclass",
        "Funny Qtype",
        "Src Port 0",
        "Malformed"
	];

my $client_subnet2_colors = [ qw(
	red
	orange
	yellow
	brightgreen
	brightblue
	darkblue
	purple
	magenta
	redorange
	yellow2
	green
	) ];

my $std_trace_yaxes = {
	rate => {
	    label => 'Query Rate (q/s)',
	    divideflag => 1,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Queries',
    	    divideflag => 0,
	    default => 0,
	},
    };

my $std_accum_yaxes = {
	rate => {
	    label => 'Mean Query Rate (q/s)',
	    divideflag => 1,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Queries',
    	    divideflag => 0,
	    default => 0,
	},
    };

%PLOTS = (

  bynode => {
    dataset 	=> 'qtype',
    datafile	=> 'qtype',
    keys	=> [ qw(a b c d e f g h i j k l m) ],
    names	=> [ qw(a b c d e f g h i j k l m) ],
    colors	=> [ qw (red orange yellow brightgreen brightblue
		     purple magenta redorange yellow2 green darkblue) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Queries by Node',
  },

  qtype => {
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Queries by QType',
    map_legend	=> 1,
  },

  dnssec_qtype => {
    dataset	=> 'qtype',
    keys	=> [ qw(24 25 30 43 46 47 48) ],
    names	=> [ qw(SIG KEY NXT DS RRSIG NSEC DNSKEY) ],
    colors	=> [ qw(yellow2 orange magenta purple brightblue brightgreen red) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'DNSSEC-related Queries by QType',
    map_legend	=> 1,
  },

  rcode => {
    keys	=> [ qw(0 3 5 8 else) ],
    names	=> [ qw(NOERROR NXDOMAIN REFUSED NXRRSET Other) ],
    colors	=> [ qw(brightgreen red blue orange purple) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Replies by Rcode',
    map_legend	=> 1,
  },

  opcode => {
    keys	=> [ qw(1 2 4 5 else) ],
    names	=> [ qw(Iquery Status Notify Update_ Other) ],
    colors	=> [ qw(orange yellow brightgreen brightblue purple red) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Messages by Opcode (excluding QUERY)',
    map_legend	=> 1,
  },

  certain_qnames_vs_qtype => {
    keys => [ qw(
        localhost:1
        localhost:28
        localhost:38
        localhost:else
        X.root-servers.net:1
        X.root-servers.net:28
        X.root-servers.net:38
        X.root-servers.net:else
    )],
    names => [
        'A? localhost',
        'AAAA? localhost',
        'A6? localhost',
        'other? localhost',
        'A? X.root-servers.net',
        'AAAA? X.root-servers.net',
        'A6? X.root-servers.net',
        'other? X.root-servers.net'
    ],
    colors => [ qw(
        skyblue
        darkblue
        drabgreen
        teal
        yellow
        orange
        pink
        red
    )],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    munge_func  => sub {
	# XXX: 'shift' represents the old data hashref
	DSC::grapher::munge_2d_to_1d(shift, [qw(localhost X.root-servers.net else)], [qw(1 28 38)])
    },
    plottitle	=> 'Queries for localhost and X.root-servers-.net',
    map_legend	=> 1,
  },

  client_subnet_accum => {
    dataset => 'client_subnet',
    datafile => 'client_subnet',
    plot_type => 'accum1d',
    keys	=> [ qw(?? IA LA AP RI AR) ],
    names	=> [ qw(Unknown IANA LACNIC APNIC RIPE ARIN) ],
    colors	=> [ qw(black red purple yellow blue brightgreen) ],
    label_func => sub {
	use IP::Country;
	my $l = new IP::Country::Fast;
	$l->inet_atocc(shift) || '??';
    },
    color_func => sub {
	use IP::Authority;
	my $l = new IP::Authority;
	$l->inet_atoauth(shift) || '??';
    },
    data_reader => \&DSC::extractor::read_data2,
    data_summer => \&DSC::grapher::data_summer_0d,
    yaxes	=> $std_accum_yaxes,
    plottitle	=> 'Busiest Client Subnets',
    map_legend	=> 1,
  },

  client_subnet2_accum => {
    dataset => 'client_subnet2',
    plot_type => 'accum2d',
    yaxes	=> $std_accum_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle	=> 'Query Classifications by Subnets',
    map_legend	=> 1,
  },

  client_subnet2_trace => {
    dataset	=> 'client_subnet2',
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    data_reader	=> \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle	=> 'Query Classifications by Subnets',
    map_legend	=> 1,
  },

  client_subnet2_count => {
    dataset	=> 'client_subnet2',
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    data_reader	=> \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> {
	rate => {
	    label => '# Client Subnets',
	    divideflag => 0,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Subnets',
    	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle	=> 'Query Classifications by Subnets',
    map_legend	=> 1,
  },

  qtype_vs_all_tld => {
    #
    # this dataset is stored as:
    #   qtype tld count
    # and we have to swap the first two dimensions
    #
    munge_func => sub {
	my $old = shift;
	my $new;
	foreach my $k1 (keys %$old) {
		foreach my $k2 (keys %{$old->{$k1}}) {
			$new->{$k2}{$k1} = $old->{$k1}{$k2};
		}
	}
	$new;
    },
    dataset => 'qtype_vs_tld',
    datafile => 'qtype_vs_tld',
    plot_type => 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $std_accum_yaxes,
    plottitle   => 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  qtype_vs_invalid_tld => {
    #
    # this dataset is stored as:
    #   qtype tld count
    # and we have to swap the first two dimensions
    #
    munge_func => sub {
	my $old = shift;
	my $new;
	foreach my $k1 (keys %$old) {
		foreach my $k2 (keys %{$old->{$k1}}) {
			next if (!DSC::grapher::invalid_tld_filter($k2));
			$new->{$k2}{$k1} = $old->{$k1}{$k2};
		}
	}
	$new;
    },
    dataset => 'qtype_vs_tld',
    datafile => 'qtype_vs_tld',
    plot_type => 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $std_accum_yaxes,
    plottitle   => 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  qtype_vs_valid_tld => {
    #
    # this dataset is stored as:
    #   qtype tld count
    # and we have to swap the first two dimensions
    #
    munge_func => sub {
	my $old = shift;
	my $new;
	foreach my $k1 (keys %$old) {
		foreach my $k2 (keys %{$old->{$k1}}) {
			next if (!DSC::grapher::valid_tld_filter($k2));
			$new->{$k2}{$k1} = $old->{$k1}{$k2};
		}
	}
	$new;
    },
    dataset => 'qtype_vs_tld',
    datafile => 'qtype_vs_tld',
    plot_type => 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $std_accum_yaxes,
    plottitle   => 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  qtype_vs_numeric_tld => {
    #
    # this dataset is stored as:
    #   qtype tld count
    # and we have to swap the first two dimensions
    #
    munge_func => sub {
	my $old = shift;
	my $new;
	foreach my $k1 (keys %$old) {
		foreach my $k2 (keys %{$old->{$k1}}) {
			next if (!DSC::grapher::numeric_tld_filter($k2));
			$new->{$k2}{$k1} = $old->{$k1}{$k2};
		}
	}
	$new;
    },
    dataset => 'qtype_vs_tld',
    datafile => 'qtype_vs_tld',
    plot_type => 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $std_accum_yaxes,
    plottitle   => 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  direction_vs_ipproto => {
    dataset => 'direction_vs_ipproto',
    plot_type => 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(recv:icmp recv:tcp recv:udp recv:else)],
    names	=> [qw(ICMP TCP UDP Other)],
    colors	=> [qw(red brightgreen purple orange)],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    yaxes	=> {
        rate => {
            label => 'Packet Rate (p/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Pakcets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Received packets by IP protocol',
    map_legend	=> 1,
    munge_func  => sub {
	# XXX: 'shift' represents the old data hashref
	DSC::grapher::munge_2d_to_1d(shift, [qw(recv)], [qw(icmp tcp udp else)])
   }
  },

  direction_vs_ipproto_sent => {
    dataset => 'direction_vs_ipproto',
    datafile => 'direction_vs_ipproto',
    plot_type => 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(sent:icmp sent:tcp sent:udp sent:else)],
    names	=> [qw(ICMP TCP UDP Other)],
    colors	=> [qw(red brightgreen purple orange)],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    yaxes	=> {
        rate => {
            label => 'Packet Rate (p/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Pakcets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Transmitted packets by IP protocol',
    map_legend	=> 1,
    munge_func  => sub {
	# XXX: 'shift' represents the old data hashref
	DSC::grapher::munge_2d_to_1d(shift, [qw(sent)], [qw(icmp tcp udp else)])
   }
  },

  query_attrs => {
    plot_type	=> 'none',
    # not a real plot, but we get errors if these members aren't present
    keys	=> [ qw(a) ],
    names	=> [ qw(a) ],
    colors	=> [ qw (red) ],
  },

  idn_qname => {
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(idn)],
    names	=> [ 'IDN Qname' ],
    colors	=> [qw(blue)],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle   => 'Queries Containing Internationalized Qnames',
  },

  rd_bit => {
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(set)],
    names	=> [ 'Recursion Desired' ],
    colors	=> [qw(purple)],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle   => 'Queries With Recursion Desired bit set',
  },

  do_bit => {
    dataset	=> 'd0_bit',
    datafile	=> 'd0_bit',
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(set)],
    names	=> [ 'DNSSEC OK' ],
    colors	=> [qw(red)],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle   => 'Queries With DNSSEC OK bit set',
  },

  edns_version => {
    keys	=> [ qw(none 0 else) ],
    names	=> [ qw(none Zero Other) ],
    colors	=> [ qw(orange yellow brightgreen brightblue purple red) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'EDNS Version Numbers',
    map_legend	=> 1,
  },

  qtype_vs_qnamelen => {
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'hist2d',
    yaxes	=> {
	count => {
	    label => 'Query Counts',
	    divideflag => 0,
	    default => 1,
	},
	percent => {
	    label => '% of Queries',
	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle	=> 'Query Name Lengths',
    map_legend	=> 1,
  },

 rcode_vs_replylen => {
    keys	=> [ qw(0 3) ],
    names	=> [ 'Rcode 0' , 'Rcode 3' ],
    colors	=> [ qw(red brightgreen) ],
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'hist2d',
    yaxes	=> {
	count => {
	    label => 'Query Counts',
	    divideflag => 0,
	    default => 1,
	},
	percent => {
	    label => '% of Queries',
	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle	=> 'Reply Code Message Lengths',
    map_legend	=> 1,
 },

  client_addr_vs_rcode_accum => {
    dataset => 'client_addr_vs_rcode',
    plot_type => 'accum2d',
    yaxes	=> $std_accum_yaxes,
    keys	=> [ qw(0 1 2 3 4 5) ],
    names	=> [ qw(NOERROR FORMERR SERVFAIL NXDOMAIN NOTIMPL REFUSED) ],
    colors	=> [ qw(brightgreen red purple blue orange magenta ) ],
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle	=> 'Rcodes and Addrs',
    map_legend	=> 1,
  },

  ipv6_rsn_abusers_accum => {
    dataset	=> 'ipv6_rsn_abusers',
    plot_type	=> 'accum1d',
    keys	=> [ qw(Other BIND8 BIND9 W2000 WINNT W2003 DJBDNS NoAns) ],
    names	=> [ qw(Other BIND8 BIND9 W2000 WINNT W2003 DJBDNS NoAns) ],
    colors	=> [ qw(red brightblue brightgreen claret orange yellow purple gray(0.5)) ],
    color_func	=> \&guess_software,
    label_func	=> sub { '.'; },
    data_reader => \&DSC::extractor::read_data2,
    data_summer => \&DSC::grapher::data_summer_0d,
    yaxes	=> $std_accum_yaxes,
    plottitle	=> 'Clients sending excessive root-servers.net queries',
    map_legend	=> 1,
  },

);

my %FPDNSCACHE;

sub fpdns_query($) {
        my $addr = shift;
	unless (tied(%FPDNSCACHE)) {
		tie(%FPDNSCACHE,
			'NDBM_File',
			'/var/tmp/fpdns-cache', 0x200 | 0x002, 0666) or die "$!";
	}
        if (defined(my $val = $FPDNSCACHE{$addr})) {
print STDERR "fpdns cache hit!\n";
                my $when = $FPDNSCACHE{"${addr}_when"};
                return $val if (time - $when  < 86400);
        }
	print STDERR "running fpdns -t 1 -s -c $addr\n";
        open(CMD, "/usr/local/bin/fpdns -t 1 -s -c $addr|") or return 'NoAns';
        my $ans = <CMD>;
	close(CMD);
        $ans =~ s/^[0-9\.]+\s+//;
        $FPDNSCACHE{"${addr}_when"} = time;
        $FPDNSCACHE{$addr} = $ans;
}

sub guess_software($) {
        my $addr = shift;
        my $fp = &fpdns_query($addr);
        return 'BIND9' if ($fp =~ /BIND 9/);
        return 'BIND8' if ($fp =~ /BIND 8/);
        return 'WINNT' if ($fp =~ /Microsoft Windows NT/);
        return 'W2000' if ($fp =~ /Microsoft Windows 2000/);
        return 'W2003' if ($fp =~ /Microsoft Windows 2003/);
        return 'NoAns' if ($fp =~ /query timed out/);
        return 'Other';
}

1;
