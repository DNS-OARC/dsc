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
use Net::DNS::Resolver;

my $qtype_keys	= [ qw(1 2  5     6   12  15 28   33  38 255 else) ];
my $qtype_names	= [ qw(A NS CNAME SOA PTR MX AAAA SRV A6 ANY Other) ];
my $qtype_colors = [ qw(red orange yellow brightgreen brightblue purple magenta redorange yellow2 green darkblue) ];
my $port_range_colors = [qw(
xrgb(FF0000) xrgb(FF2200) xrgb(FF4100) xrgb(FF6300) xrgb(FF7600) xrgb(FF8500) xrgb(FF9600) xrgb(FFA600)
xrgb(FFB500) xrgb(FFC700) xrgb(FFD800) xrgb(FFE700) xrgb(FFF800) xrgb(EBFF00) xrgb(CCFF00) xrgb(AAFF00)
xrgb(77FF00) xrgb(33FF00) xrgb(14E000) xrgb(00C41B) xrgb(00B560) xrgb(00A682) xrgb(008F9E) xrgb(006EB0)
xrgb(004FBF) xrgb(0330C9) xrgb(141EB8) xrgb(230FA8) xrgb(340099) xrgb(3D0099) xrgb(4D0099) xrgb(660099)
)];
my $twenty_colors = [qw(
xrgb(ff0000) xrgb(ff9900) xrgb(ffff00) xrgb(00cc00) xrgb(0033cc) xrgb(660099)
xrgb(ff3300) xrgb(ffb200) xrgb(ccff00) xrgb(00b366) xrgb(1919b3) xrgb(990099)
xrgb(ff6600) xrgb(ffcc00) xrgb(99ff00) xrgb(009999) xrgb(330099) xrgb(cc0099)
xrgb(ff8000) xrgb(ffe500) xrgb(33ff00) xrgb(0066b3) xrgb(400099) xrgb(e60066)
xrgb(ff0000) xrgb(ff9900) xrgb(ffff00) xrgb(00cc00) xrgb(0033cc) xrgb(660099)
xrgb(ff3300) xrgb(ffb200) xrgb(ccff00) xrgb(00b366) xrgb(1919b3) xrgb(990099)
xrgb(ff6600) xrgb(ffcc00) xrgb(99ff00) xrgb(009999) xrgb(330099) xrgb(cc0099)
xrgb(ff8000) xrgb(ffe500) xrgb(33ff00) xrgb(0066b3) xrgb(400099) xrgb(e60066)
)];
my $port_range_keys = [ qw(
0-2047 2048-4095 4096-6143 6144-8191 8192-10239 10240-12287 12288-14335 14336-16383
16384-18431 18432-20479 20480-22527 22528-24575 24576-26623 26624-28671 28672-30719 30720-32767
32768-34815 34816-36863 36864-38911 38912-40959 40960-43007 43008-45055 45056-47103 47104-49151
49152-51199 51200-53247 53248-55295 55296-57343 57344-59391 59392-61439 61440-63487 63488-65535
)];

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

my $pct_trace_yaxes = {
	# just like $std_trace_yaxes, but we want percent to be the default
	rate => {
	    label => 'Query Rate (q/s)',
	    divideflag => 1,
	    default => 0,
	},
	percent => {
	    label => 'Percent of Queries',
    	    divideflag => 0,
	    default => 1,
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

  bynode => {
    dataset 	=> 'qtype',
    datafile	=> 'qtype',
    keys	=> [ qw(a b c d e f g h i j k l m n o) ],
    names	=> [ qw(a b c d e f g h i j k l m n o) ],
#    colors	=> [ qw (red orange yellow brightgreen brightblue
#		     purple magenta redorange yellow2 green darkblue
#		     yelloworange powderblue claret lavender ) ],
    colors => $twenty_colors,
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
    keys	=> [ qw(24 25 30 43 46 47 50 48) ],
    names	=> [ qw(SIG KEY NXT DS RRSIG NSEC NSEC3 DNSKEY) ],
    colors	=> [ qw(yellow2 orange magenta purple brightblue brightgreen darkblue red) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    plot_type	=> 'trace',
    yaxes	=> $std_trace_yaxes,
    plottitle	=> 'DNSSEC-related Queries by QType',
    map_legend	=> 1,
  },

  rcode => {
    keys	=> [ qw(0 2 3 5 8 else) ],
    names	=> [ qw(NOERROR SERVFAIL NXDOMAIN REFUSED NXRRSET Other) ],
    colors	=> [ qw(brightgreen yellow red blue orange purple) ],
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
	my $self = shift;
	my $data = shift;
	$self->munge_2d_to_1d($data, [qw(localhost X.root-servers.net else)], [qw(1 28 38)])
    },
    plottitle	=> 'Queries for localhost and X.root-servers.net',
    map_legend	=> 1,
  },

  client_subnet_accum => {
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
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
    yaxes	=> $std_accum_yaxes,
    plottitle	=> 'Busiest Client Subnets',
    map_legend	=> 1,
  },

#  client_subnet_count => {
#    dataset => 'client_subnet',
#    plot_type => 'trace',
#    keys	=> [ qw(All) ],
#    names	=> [ qw(all) ],
#    colors	=> [ qw(red) ],
#    data_reader => \&DSC::extractor::read_data2,
#    data_summer => \&DSC::grapher::data_summer_0d,
#    munge_func	=> \&DSC::grapher::munge_anonymize_ip,
#    yaxes	=> $std_trace_yaxes,
#    plottitle	=> '# /24s seen per minute',
#    map_legend	=> 0,
#  },

  client_subnet2_accum => {
    plot_type => 'accum2d',
    yaxes	=> $std_accum_yaxes,
    keys	=> $client_subnet2_keys,
    names	=> $client_subnet2_names,
    colors	=> $client_subnet2_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle	=> 'Query Classifications by Subnets',
    map_legend	=> 1,
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  client_subnet2_trace => {
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
    plot_type	=> 'trace',
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
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  qtype_vs_all_tld => {
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
    munge_func => sub {
      my $self = shift;
      my $data = shift;
      foreach my $k1 (keys %$data) {
	delete $data->{$k1} unless ($self->invalid_tld_filter($k1));
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
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
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
    plottitle   => 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  qtype_vs_valid_tld => {
    munge_func => sub {
      my $self = shift;
      my $data = shift;
      foreach my $k1 (keys %$data) {
	delete $data->{$k1} unless ($self->valid_tld_filter($k1));
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
    data_reader	=> \&DSC::extractor::read_data3,
    data_summer	=> \&DSC::grapher::data_summer_1d,
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
    plottitle  	=> 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  qtype_vs_numeric_tld => {
    munge_func => sub {
      my $self = shift;
      my $data = shift;
      foreach my $k1 (keys %$data) {
	delete $data->{$k1} unless ($self->numeric_tld_filter($k1));
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
    data_reader	=> \&DSC::extractor::read_data3,
    data_summer	=> \&DSC::grapher::data_summer_1d,
    yaxes	=> $std_accum_yaxes,
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
    plottitle  	=> 'Most Popular TLDs Queried',
    map_legend	=> 1,
  },

  direction_vs_ipproto => {
    dataset => 'direction_vs_ipproto',
    plot_type => 'trace',
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
            label => 'Percent of Packets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Received packets by IP protocol',
    map_legend	=> 1,
    munge_func  => sub {
	my $self = shift;
	my $data = shift;
	$self->munge_2d_to_1d($data, [qw(recv)], [qw(icmp tcp udp else)])
   }
  },

  dns_ip_version => {
    plot_type   => 'trace',
    keys	=> [qw(IPv4 IPv6 else)],
    names	=> [qw(IPv4 IPv6 Other)],
    colors	=> [qw(red brightgreen purple)],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    yaxes	=> {
        rate => {
            label => 'Queries Rate (q/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Queries',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'IP Version Carrying DNS Queries',
    map_legend	=> 1,
    munge_func  => \&DSC::grapher::munge_sum_2d_to_1d,
  },

  dns_ip_version_vs_qtype => {
    plot_type => 'accum2d',
    divideflag 	=> 0,
    keys	=> $qtype_keys,
    names	=> $qtype_names,
    colors	=> $qtype_colors,
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $std_accum_yaxes,
    plottitle   => 'Query Types by IP version',
    map_legend	=> 1,
  },

  transport_vs_qtype => {
    dataset => 'transport_vs_qtype',
    plot_type => 'trace',
    keys	=> [qw(tcp udp else)],
    names	=> [qw(TCP UDP Other)],
    colors	=> [qw(red brightgreen purple)],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    yaxes	=> {
        rate => {
            label => 'Queries Rate (q/s)',
            divideflag => 1,
            default => 1,
        },
        percent => {
            label => 'Percent of Queries',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Transports Carrying DNS Queries',
    map_legend	=> 1,
    munge_func  => \&DSC::grapher::munge_sum_2d_to_1d,
  },

  direction_vs_ipproto_sent => {
    dataset => 'direction_vs_ipproto',
    datafile => 'direction_vs_ipproto',
    plot_type => 'trace',
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
            label => 'Percent of Packets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'Transmitted packets by IP protocol',
    map_legend	=> 1,
    munge_func  => sub {
	my $self = shift;
	my $data = shift;
	$self->munge_2d_to_1d($data, [qw(sent)], [qw(icmp tcp udp else)])
   }
  },

  direction => {
    dataset => 'direction_vs_ipproto',
    datafile => 'direction_vs_ipproto',
    plot_type => 'trace',
    keys	=> [qw(sent recv else)],
    names	=> [qw(Sent Recv Other)],
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
            label => 'Percent of Packets',
            divideflag => 0,
            default => 0,
        },
    },
    plottitle   => 'IP packet direction',
    map_legend	=> 1,
    munge_func  => sub {
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
    dataset	=> 'do_bit',
    datafile	=> 'do_bit',
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
    xaxislabel	=> 'Name Length (bytes)',
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
    xaxislabel	=> 'DNS Message Size (bytes)',
    map_legend	=> 1,
 },

  client_addr_vs_rcode_accum => {
    plot_type => 'accum2d',
    yaxes	=> $std_accum_yaxes,
    keys	=> [ qw(0 1 2 3 4 5) ],
    names	=> [ qw(NOERROR FORMERR SERVFAIL NXDOMAIN NOTIMPL REFUSED) ],
    colors	=> [ qw(brightgreen red purple blue orange magenta ) ],
    data_reader => \&DSC::extractor::read_data3,
    data_summer => \&DSC::grapher::data_summer_1d,
    plottitle	=> 'Rcodes and Addrs',
    map_legend	=> 1,
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  ipv6_rsn_abusers_accum => {
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
    munge_func 	=> \&DSC::grapher::munge_anonymize_ip,
  },

  chaos_types_and_names => {
    plot_type => 'trace',
    yaxes	=> $std_trace_yaxes,
    keys	=> [qw(16:version.bind 16:hostname.bind Other)],
    names	=> ['TXT version.bind', 'TXT hostname.bind', 'Other'],
    colors	=> [qw(red brightgreen purple)],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    plottitle   => 'CHAOS Queries',
    map_legend	=> 1,
    munge_func  => sub {
	my $self = shift;
	my $data = shift;
	$self->munge_2d_to_1d($data, [qw(16)], [qw(version.bind hostname.bind else)])
   }
  },

  client_ports_count => {
    dataset => 'client_ports',
    plot_type => 'trace',
    keys	=> [ qw(All) ],
    names	=> [ qw(all) ],
    colors	=> [ qw(red) ],
    data_reader => \&DSC::extractor::read_data2,
    data_summer => \&DSC::grapher::data_summer_0d,
    yaxes	=> {
	rate => {
	    label => '# Unique Ports',
	    divideflag => 0,
	    default => 1,
	},
    },
    plottitle	=> '# unique ports seen per minute',
    map_legend	=> 0,
    data_dim    => 1,
  },

  client_port_range => {
    dataset => 'client_port_range',
    plot_type => 'trace',
    keys	=> $port_range_keys,
    names	=> $port_range_keys,
    colors	=> $port_range_colors,
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $pct_trace_yaxes,
    plottitle	=> 'Port Range',
    map_legend	=> 0,
    # ARGH, Ploticus only supports stacking of 40 fields, so
    # here we change the original 64 bins into 32 bins.
    munge_func  => sub {
        my $self = shift;
        my $data = shift;
        my %newdata;
	for (my $fbin = 0; $fbin < 64; $fbin++) {
		my $tbin = $fbin >> 1;
		my $fkey = sprintf "%d-%d", $fbin << 10, (($fbin + 1) << 10) - 1;
		my $tkey = sprintf "%d-%d", $tbin << 11, (($tbin + 1) << 11) - 1;
        	foreach my $t (keys %$data) {
			$newdata{$t}{$tkey} += $data->{$t}{$fkey} if $data->{$t}{$fkey};
		}
	}
        \%newdata;
   }
  },

  edns_bufsiz => {
    debug => 1,
    dataset => 'edns_bufsiz',
    plot_type => 'trace',
    keys	=> [qw(None 0-511 512-1023 1024-1535 1536-2047 2048-2559 2560-3071 3072-3583 3584-4095 4096-4607)],
    names	=> [qw(None 0-511 512-1023 1024-1535 1536-2047 2048-2559 2560-3071 3072-3583 3584-4095 4096-4607)],
    colors	=> [ qw(red orange yellow brightgreen brightblue purple magenta redorange yellow2 green darkblue) ],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes	=> $pct_trace_yaxes,
    plottitle	=> 'EDNS Buffer Sizes',
    map_legend	=> 0,
  },

  second_ld_vs_rcode_accum => {
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

  third_ld_vs_rcode_accum => {
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

  pcap_stats => {
    dataset => 'pcap_stats',
    plot_type => 'trace',
    keys       => [qw(filter_received pkts_captured kernel_dropped)],
    names      => [qw(recv capt drop)],
    colors     => [qw(red brightgreen purple )],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    yaxes      => {
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
    plottitle   => 'pcap statistics',
    map_legend => 1,
    munge_func  => \&DSC::grapher::munge_sum_2d_to_1d,
  },

  priming_queries => {
    dataset => 'priming_queries',
    plot_type => 'trace',
    keys       => [qw(udp tcp)],
    names      => [qw(UDP TCP)],
    colors     => [qw( red brightgreen )],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
    yaxes	=> $std_trace_yaxes,
    plottitle   => 'Priming Queries',
    map_legend => 1,
    munge_func  => \&DSC::grapher::munge_sum_2d_to_1d,
  },

  priming_responses => {
    dataset => 'priming_responses',
    plot_type => 'trace_line',
    keys       => [qw(min mean max)],
    names      => [qw(Min Mean Max)],
    colors     => [qw( red brightgreen purple )],
    data_reader => \&DSC::extractor::read_data,
    data_summer => \&DSC::grapher::data_summer_1d,
    yaxes      => {
	rate => {
	    label => 'Response Size',
	    divideflag => 0,
	    default => 1,
	},
    },
    plottitle   => 'Priming Responses',
    map_legend => 1,
    munge_func  => \&DSC::grapher::munge_min_max_mean,
  },

  reflector_attack => {
    plot_type => 'trace',
    keys	=> [qw(recv:qr=0,aa=0 recv:qr=0,aa=1 recv:qr=1,aa=0 recv:qr=1,aa=1)],
    names	=> [qw(QR=0,AA=0 QR=0,AA=1 QR=1,AA=0 QR=1,AA=1)],
    colors	=> [qw(brightgreen purple red orange)],
    data_reader => \&DSC::extractor::read_data4,
    data_summer => \&DSC::grapher::data_summer_2d,
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
    plottitle   => 'Received packets by IP QR/AA bits',
    map_legend	=> 1,
    munge_func  => sub {
	my $self = shift;
	my $data = shift;
	$self->munge_2d_to_1d($data, [qw(recv)], [qw(qr=0,aa=0 qr=0,aa=1 qr=1,aa=0 qr=1,aa=1)])
   }
  },

);

my %FPDNSCACHE;

sub fpdns_query($) {
        my $addr = shift;
	my $noans = 'NoAns';
	my $res = Net::DNS::Resolver->new;
	my $qname = join('.', reverse(split(/\./, $addr)), 'fpdns.measurement-factory.com');
	my $pkt = $res->query($qname, 'TXT');
	return $noans unless $pkt;
	return $noans unless $pkt->answer;
	foreach my $rr ($pkt->answer) {
		next unless $rr->type eq 'TXT';
		return $rr->txtdata;
	}
	return $noans;
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
