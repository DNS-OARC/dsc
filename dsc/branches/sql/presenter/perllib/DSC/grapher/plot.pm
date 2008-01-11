package DSC::grapher::plot;

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
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

use strict;
use warnings;
use IO::Socket::INET;

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
        "Unclassified",
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
	count => {
	    label => 'Number of Queries',
    	    divideflag => 0,
	    default => 0,
	},
    };

%PLOTS = (

  byserver => {
    dataset 	=> 'qtype',
    datafile	=> 'qtype',
    keys	=> [ qw(a b c d e f g h i j k l m n o) ],
    names	=> [ qw(a b c d e f g h i j k l m n o) ],
    colors	=> [ qw (red orange yellow brightgreen brightblue
		     purple magenta redorange yellow2 green darkblue
		     yelloworange powderblue claret lavender ) ],
    dbkeys      => [ 'start_time', 'server_id' ],
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Queries by Server',
    menutitle	=> 'By Server',
  },

  bynode => {
    dataset 	=> 'qtype',
    datafile	=> 'qtype',
    keys	=> [ qw(a b c d e f g h i j k l m n o) ],
    names	=> [ qw(a b c d e f g h i j k l m n o) ],
    colors	=> [ qw (red orange yellow brightgreen brightblue
		     purple magenta redorange yellow2 green darkblue
		     yelloworange powderblue claret lavender ) ],
    dbkeys      => [ 'start_time', 'node_id' ],
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Queries by Node',
    menutitle	=> 'By Node',
  },

  qtype => {
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Queries by QType',
    menutitle	=> 'Common Qtypes',
    map_legend	=> 1,
  },

  dnssec_qtype => {
    dataset	=> 'qtype',
    keys	=> [ qw(24 25 30 43 46 47 48) ],
    names	=> [ qw(SIG KEY NXT DS RRSIG NSEC DNSKEY) ],
    colors	=> [ qw(yellow2 orange magenta purple brightblue brightgreen red) ],
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'DNSSEC-related Queries by QType',
    menutitle	=> 'DNSSEC Qtypes',
    map_legend	=> 1,
  },

  rcode => {
    keys	=> [ qw(0 2 3 5 8 else) ],
    names	=> [ qw(NOERROR SERVFAIL NXDOMAIN REFUSED NXRRSET Other) ],
    colors	=> [ qw(brightgreen yellow red blue orange purple) ],
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Replies by Rcode',
    menutitle	=> 'Rcodes',
    map_legend	=> 1,
  },

  opcode => {
    keys	=> [ qw(1 2 4 5 else) ],
    names	=> [ qw(Iquery Status Notify Update_ Other) ],
    colors	=> [ qw(orange yellow brightgreen brightblue purple red) ],
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'Messages by Opcode (excluding QUERY)',
    menutitle	=> 'Opcodes',
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
    dbkeys      => [ 'start_time', 'key1', 'key2' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    munge_func  => sub {
	# XXX: 'shift' represents the old data hashref
	DSC::grapher::munge_2d_to_1d(shift, [qw(localhost X.root-servers.net else)], [qw(1 28 38)])
    },
    plottitle	=> 'Queries for localhost and X.root-servers-.net',
    menutitle	=> 'Popular Names',
    map_legend	=> 1,
  },

  client_subnet_accum => {
    dataset => 'client_subnet',
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
    dbkeys      => [ 'key1' ],
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
    yaxes	=> $std_accum_yaxes,
    plottitle	=> 'Busiest Client Subnets',
    menutitle	=> 'Client Geography',
    map_legend	=> 1,
  },

#  client_subnet_count => {
#    dataset => 'client_subnet',
#    plot_type => 'trace',
#    keys	=> [ qw(All) ],
#    names	=> [ qw(all) ],
#    colors	=> [ qw(red) ],
#    dbkeys      => [ 'start_time', 'key1' ],
#    munge_func	=> \&DSC::grapher::munge_anonymize_ip,
#    yaxes	=> $std_trace_yaxes,
#    plottitle	=> '# /24s seen per minute',
#    menutitle	=> 'Subnets',
#    map_legend	=> 0,
#  },

  client_subnet2_accum => {
    dataset => 'client_subnet2',
    plot_type => 'accum2d',
    yaxes	=> $std_accum_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    dbkeys      => [ 'key1', 'key2' ],
    plottitle	=> 'Query Classifications of Busiest Clients',
    menutitle	=> 'Classification by Client',
    map_legend	=> 1,
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  client_subnet2_trace => {
    dataset	=> 'client_subnet2',
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    dbkeys      => [ 'start_time', 'key1' ],
    plottitle	=> 'Query Classifications over Time',
    menutitle	=> 'Query Classification',
    map_legend	=> 1,
  },

  client_subnet2_count => {
    dataset	=> 'client_subnet2',
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    dbkeys      => [ 'start_time', 'key1' ],
    yaxes	=> {
	rate => {
	    label => '# of Client Subnets',
	    divideflag => 0,
	    default => 1,
	},
### percent is broken.
#	percent => {
#	    label => '% of Client Subnets',
#    	    divideflag => 0,
#	    default => 0,
#	},
    },
    plottitle	=> 'Client Subnets per Query Classification over Time',
    menutitle	=> 'Clients per Classification',
    map_legend	=> 1,
  },

  qtype_vs_all_tld => {
    dataset => 'qtype_vs_tld',
    datafile => 'qtype_vs_tld',
    plot_type => 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    dbkeys      => [ 'key1', 'key2' ],
    yaxes	=> $std_accum_yaxes,
    plottitle   => 'Most-Queried TLDs',
    menutitle	=> 'Most-Queried TLDs',
    map_legend	=> 1,
  },

  qtype_vs_invalid_tld => {
    munge_func => sub {
      my $data = shift;
      foreach my $k1 (keys %$data) {
	delete $data->{$k1} unless (DSC::grapher::invalid_tld_filter($k1));
      }
      $data;
    },
    dataset	=> 'qtype_vs_tld',
    datafile	=> 'qtype_vs_tld',
    plot_type	=> 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    dbkeys      => [ 'key1', 'key2' ],
    yaxes	=>  {
	rate => {
	    label => 'Mean Query Rate (q/s)',
	    divideflag => 1,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Invalid Queries',
    	    divideflag => 0,
	    default => 0,
	},
	count => {
	    label => 'Number of Queries',
    	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle   => 'Most-Queried Invalid TLDs',
    menutitle	=> 'Invalid TLDs',
    map_legend	=> 1,
  },

  qtype_vs_valid_tld => {
    munge_func => sub {
      my $data = shift;
      foreach my $k1 (keys %$data) {
	delete $data->{$k1} unless (DSC::grapher::valid_tld_filter($k1));
      }
      $data;
    },
    dataset	=> 'qtype_vs_tld',
    datafile	=> 'qtype_vs_tld',
    plot_type	=> 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    dbkeys      => [ 'key1', 'key2' ],
    yaxes	=>  {
	rate => {
	    label => 'Mean Query Rate (q/s)',
	    divideflag => 1,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Valid Queries',
    	    divideflag => 0,
	    default => 0,
	},
	count => {
	    label => 'Number of Queries',
    	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle  	=> 'Most-Queried Valid TLDs',
    menutitle	=> 'Valid TLDs',
    map_legend	=> 1,
  },

  qtype_vs_numeric_tld => {
    munge_func => sub {
      my $data = shift;
      foreach my $k1 (keys %$data) {
	delete $data->{$k1} unless (DSC::grapher::numeric_tld_filter($k1));
      }
      $data;
    },
    dataset	=> 'qtype_vs_tld',
    datafile	=> 'qtype_vs_tld',
    plot_type	=> 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    dbkeys      => [ 'key1', 'key2' ],
    yaxes	=>  {
	rate => {
	    label => 'Mean Query Rate (q/s)',
	    divideflag => 1,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Numeric Queries',
    	    divideflag => 0,
	    default => 0,
	},
	count => {
	    label => 'Number of Queries',
    	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle  	=> 'Most-Queried Numeric TLDs',
    menutitle	=> 'Numeric TLDs',
    map_legend	=> 1,
  },

  direction_vs_ipproto => {
    dataset => 'direction_vs_ipproto',
    plot_type => 'trace',
    keys	=> [qw(icmp tcp udp else)],
    names	=> [qw(ICMP TCP UDP Other)],
    colors	=> [qw(red brightgreen purple orange)],
    dbkeys      => [ 'start_time', 'key2' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    where       => "key1 = 'recv'",
    yaxes	=> {
        rate => {
            label => 'Packet Rate (p/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Packets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Received packets by IP protocol',
    menutitle	=> 'IP Protocols Received',
    map_legend	=> 1,
  },

  direction_vs_ipproto_sent => {
    dataset => 'direction_vs_ipproto',
    datafile => 'direction_vs_ipproto',
    plot_type => 'trace',
    keys	=> [qw(icmp tcp udp else)],
    names	=> [qw(ICMP TCP UDP Other)],
    colors	=> [qw(red brightgreen purple orange)],
    dbkeys      => [ 'start_time', 'key2' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    where       => "key1 = 'sent'",
    yaxes	=> {
        rate => {
            label => 'Packet Rate (p/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Packets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Transmitted packets by IP protocol',
    menutitle	=> 'IP Protocols Sent',
    map_legend	=> 1,
  },

  direction => {
    dataset => 'direction_vs_ipproto',
    datafile => 'direction_vs_ipproto',
    plot_type => 'trace',
    keys	=> [qw(sent recv else)],
    names	=> [qw(Sent Recv Other)],
    colors	=> [qw(red brightgreen purple orange)],
    dbkeys      => [ 'start_time', 'key1' ],
    yaxes	=> {
        rate => {
            label => 'Packet Rate (p/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Packets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'IP packet direction',
    menutitle	=> 'IP packet direction',
    map_legend	=> 1,
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
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plottitle   => 'Queries Containing Internationalized Qnames',
    menutitle	=> 'IDN Qnames',
  },

  rd_bit => {
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(set)],
    names	=> [ 'Recursion Desired' ],
    colors	=> [qw(purple)],
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plottitle   => 'Queries with Recursion Desired bit set',
    menutitle	=> 'RD bit',
  },

  do_bit => {
    dataset	=> 'do_bit',
    datafile	=> 'do_bit',
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(set)],
    names	=> [ 'DNSSEC OK' ],
    colors	=> [qw(red)],
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plottitle   => 'Queries with DNSSEC OK bit set',
    menutitle	=> 'DO bit',
  },

  edns_version => {
    keys	=> [ qw(none 0 else) ],
    names	=> [ qw(none Zero Other) ],
    colors	=> [ qw(orange yellow brightgreen brightblue purple red) ],
    dbkeys      => [ 'start_time', 'key1' ],
    nogroup     => 1, # all columns are selected or constrained to single value
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'EDNS Version Numbers',
    menutitle	=> 'EDNS Version',
    map_legend	=> 1,
  },

  qtype_vs_qnamelen => {
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    dbkeys      => [ 'key1', 'key2' ],
    plot_type	=> 'hist2d',
    yaxes	=> {
	count => {
	    label => 'Query Counts',
	    divideflag => 0,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Queries',
	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle	=> 'Query Name Lengths',
    menutitle	=> 'Qname Length',
    xaxislabel	=> 'Name Length (bytes)',
    map_legend	=> 1,
  },

 rcode_vs_replylen => {
    keys	=> [ qw(0 3) ],
    names	=> [ 'Rcode 0' , 'Rcode 3' ],
    colors	=> [ qw(red brightgreen) ],
    dbkeys      => [ 'key1', 'key2' ],
    plot_type	=> 'hist2d',
    yaxes	=> {
	count => {
	    label => 'Query Counts',
	    divideflag => 0,
	    default => 1,
	},
	percent => {
	    label => 'Percent of Queries',
	    divideflag => 0,
	    default => 0,
	},
    },
    plottitle	=> 'Reply Code Message Lengths',
    menutitle	=> 'Reply Length',
    xaxislabel	=> 'DNS Message Size (bytes)',
    map_legend	=> 1,
 },

  client_addr_vs_rcode_accum => {
    dataset => 'client_addr_vs_rcode',
    plot_type => 'accum2d',
    yaxes	=> $std_accum_yaxes,
    keys	=> [ qw(0 1 2 3 4 5) ],
    names	=> [ qw(NOERROR FORMERR SERVFAIL NXDOMAIN NOTIMPL REFUSED) ],
    colors	=> [ qw(brightgreen red purple blue orange magenta ) ],
    dbkeys      => [ 'key1', 'key2' ],
    plottitle	=> 'Rcodes by Client Address',
    menutitle	=> 'Rcodes by Client Address',
    map_legend	=> 1,
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  ipv6_rsn_abusers_accum => {
    dataset	=> 'ipv6_rsn_abusers',
    plot_type	=> 'accum1d',
    keys	=> [ qw(Other BIND8 BIND9 W2000 WINNT W2003 DJBDNS NoAns) ],
    names	=> [ qw(Other BIND8 BIND9 W2000 WINNT W2003 DJBDNS NoAns) ],
    colors	=> [ qw(red brightblue brightgreen claret orange yellow purple gray(0.5)) ],
    color_func	=> \&guess_software,
    label_func	=> sub { '.'; },
    dbkeys      => [ 'key1' ],
    yaxes	=> $std_accum_yaxes,
    plottitle	=> 'Clients sending excessive root-servers.net queries',
    menutitle	=> 'IPv6 Root Abusers',
    map_legend	=> 1,
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  chaos_types_and_names => {
    plot_type => 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(16:version.bind 16:hostname.bind Other)],
    names	=> ['TXT version.bind', 'TXT hostname.bind', 'Other'],
    colors	=> [qw(red brightgreen purple)],
    dbkeys      => [ 'start_time', 'key1', 'key2' ],
    plottitle   => 'CHAOS Queries',
    menutitle	=> 'CHAOS',
    map_legend	=> 1,
    munge_func  => sub {
	# XXX: 'shift' represents the old data hashref
	DSC::grapher::munge_2d_to_1d(shift, [qw(16)], [qw(version.bind hostname.bind else)])
   }
  },

);

my %FPDNSCACHE;

sub fpdns_query($) {
        my $addr = shift;
	my $ans = 'NoAns';
	my $sock = IO::Socket::INET->new("dns-oarc.measurement-factory.com:8053");
	return $ans unless ($sock);
	print $sock "$addr\n";
	my $rin = '';
	my $rout = '';
	vec($rin,fileno($sock),1) = 1;
	select($rout=$rin, undef, undef, 1.0);
	if (vec($rout,fileno($sock),1)) {
		$ans = <$sock>;
		chomp($ans);
	}
	close($sock);
	$ans;
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
