package DSC::extractor::config;

BEGIN {
	use Exporter   ();
	use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
	$VERSION     = 1.00;
	@ISA         = qw(Exporter);
	@EXPORT      = qw(
	        %DATASETS
	);
	%EXPORT_TAGS = ( );
	@EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

use strict;
use warnings;

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

%DATASETS = (

  qtype => {
    ndim	=> 1,
    type1	=> 'Qtype',
    outputs	=> {
      qtype => {
	keys	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
      dnssec_qtype => {
	keys	=> [ qw(24 25 30 43 46 47 48) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  rcode => {
    ndim	=> 1,
    type1	=> 'Rcode',
    outputs	=> {
      rcode => {
	keys	=> [ qw(0 2 3 5 8 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  opcode => {
    ndim	=> 1,
    type1	=> 'Opcode',
    outputs	=> {
      opcode => {
	keys	=> [ qw(0 1 2 4 5 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  edns_version => {
    ndim	=> 1,
    type1	=> 'EDNSVersion',
    outputs	=> {
      edns_version => {
	keys	=> [ qw(none 0 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  rd_bit => {
    ndim	=> 1,
    type1	=> 'RD',
    outputs	=> {
      rd_bit => {
	keys	=> [ qw(set clr) ],
	data_merger => \&main::merge_trace,
	data_reader => \&DSC::extractor::read_data,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  idn_qname => {
    ndim	=> 1,
    type1	=> 'IDNQname',
    outputs	=> {
      idn_qname => {
	keys	=> [ qw(normal idn) ],
	data_merger => \&main::merge_trace,
	data_reader => \&DSC::extractor::read_data,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  do_bit => {
    ndim	=> 1,
    type1	=> 'D0',	## legacy typo!
    outputs	=> {
      do_bit => {
	keys	=> [ qw(set clr) ],
	data_merger => \&main::merge_trace,
	data_reader => \&DSC::extractor::read_data,
	data_writer => \&DSC::extractor::write_data,
      },
    },
  },

  client_subnet => {
    ndim	=> 1,
    type1	=> 'ClientSubnet',
    outputs	=> {
      client_subnet_count => {
	data_munger	=> \&main::accum1d_to_count,
	data_reader	=> \&DSC::extractor::read_data2,
	data_merger	=> \&main::merge_trace,
	data_writer	=> \&DSC::extractor::write_data2,
      },
      client_subnet_accum => {
	data_merger	=> \&main::merge_accum1d,
	data_reader	=> \&DSC::extractor::read_data2,
	data_writer	=> \&DSC::extractor::write_data2,
      },
    },
  },

  client_subnet2 => {
    ndim	=> 2,
    type1	=> 'Class',
    type2	=> 'ClientSubnet',
    outputs	=> {
      client_subnet2_trace => {
	keys	=> $client_subnet2_keys,
	data_munger => \&main::accum2d_to_trace,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
      client_subnet2_count => {
	keys	=> $client_subnet2_keys,
	data_munger => \&main::accum2d_to_count,
	data_reader => \&DSC::extractor::read_data,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data,
      },
      client_subnet2_accum => {
	keys	=> $client_subnet2_keys,
	data_munger => \&main::swap_dimensions,
	data_reader => \&DSC::extractor::read_data3,
	data_merger => \&main::merge_accum2d,
	data_writer => \&DSC::extractor::write_data3,
	data_trimer => \&main::trim_accum2d,
      },
    },
  },

  certain_qnames_vs_qtype => {
    ndim	=> 2,
    type1	=> 'CertainQnames',
    type2	=> 'Qtype',
    outputs	=> {
      certain_qnames_vs_qtype => {
	keys2	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data4,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data4,
      },
    },
  },

  qtype_vs_tld => {
    ndim	=> 2,
    type1	=> 'Qtype',
    type2	=> 'TLD',
    outputs	=> {
      qtype_vs_tld => {
	keys2	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger	=> \&main::swap_dimensions,
	data_reader	=> \&DSC::extractor::read_data3,
	data_merger	=> \&main::merge_accum2d,
	data_writer	=> \&DSC::extractor::write_data3,
	data_trimer	=> \&main::trim_accum2d,
      },
    },
  },

  qtype_vs_domain => {
    ndim	=> 2,
    type1	=> 'Qtype',
    type2	=> 'Domain',
    outputs	=> {
      qtype_vs_domain => {
	keys2	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger	=> \&main::swap_dimensions,
	data_reader	=> \&DSC::extractor::read_data3,
	data_merger	=> \&main::merge_accum2d,
	data_writer	=> \&DSC::extractor::write_data3,
	data_trimer	=> \&main::trim_accum2d,
      },
    },
  },

  client_addr_vs_rcode => {
    ndim	=> 2,
    type1	=> 'Rcode',
    type2	=> 'ClientAddr',
    outputs	=> {
      client_addr_vs_rcode_accum => {
	data_munger => \&main::swap_dimensions,
	data_reader => \&DSC::extractor::read_data3,
	data_merger => \&main::merge_accum2d,
	data_writer => \&DSC::extractor::write_data3,
      },
    },
  },

  direction_vs_ipproto => {
    ndim	=> 2,
    type1	=> 'Direction',
    type2	=> 'IPProto',
    outputs	=> {
      direction_vs_ipproto => {
	keys2	=> [ qw(icmp tcp udp) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data4,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data4,
      },
    },
  },

  idn_vs_tld => {
    ndim	=> 1,
    type1	=> 'TLD',
    outputs	=> {
      idn_vs_tld => {
	data_reader	=> \&DSC::extractor::read_data2,
	data_merger	=> \&main::merge_accum1d,
	data_writer	=> \&DSC::extractor::write_data2,
      },
    },
  },

  ipv6_rsn_abusers => {
    ndim	=> 1,
    type1	=> 'ClientAddr',
    outputs	=> {
      ipv6_rsn_abusers_count => {
	data_munger	=> \&main::accum1d_to_count,
	data_reader	=> \&DSC::extractor::read_data2,
	data_merger	=> \&main::merge_trace,
	data_writer	=> \&DSC::extractor::write_data2,
      },
      ipv6_rsn_abusers_accum => {
	data_reader	=> \&DSC::extractor::read_data2,
	data_merger	=> \&main::merge_accum1d,
	data_writer	=> \&DSC::extractor::write_data2,
      },
    },
  },

  qtype_vs_qnamelen => {
    ndim	=> 2,
    type1	=> 'Qtype',
    type2	=> 'QnameLen',
    outputs	=> {
      qtype_vs_qnamelen => {
	data_reader => \&DSC::extractor::read_data3,
	data_merger => \&main::merge_accum2d,
	data_writer => \&DSC::extractor::write_data3,
      },
    },
  },

  rcode_vs_replylen => {
    ndim	=> 2,
    type1	=> 'Rcode',
    type2	=> 'ReplyLen',
    outputs	=> {
      rcode_vs_replylen => {
	data_reader => \&DSC::extractor::read_data3,
	data_merger => \&main::merge_accum2d,
	data_writer => \&DSC::extractor::write_data3,
      },
    },
  },

  chaos_types_and_names => {
    ndim	=> 2,
    type1	=> 'Qtype',
    type2	=> 'Qname',
    outputs	=> {
      chaos_types_and_names => {
	keys2	=> [ qw(hostname.bind version.bind other) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data4,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data4,
      },
    },
  },

  transport_vs_qtype => {
    ndim	=> 2,
    type1	=> 'Transport',
    type2	=> 'Qtype',
    outputs	=> {
      transport_vs_qtype => {
	keys2	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data4,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data4,
      },
    },
  },

  dns_ip_version_vs_qtype => {
    ndim	=> 2,
    type1	=> 'IPVersion',
    type2       => 'Qtype',
    outputs	=> {
      dns_ip_version => {
	keys2	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger => \&main::munge_elsify,
	data_reader => \&DSC::extractor::read_data4,
	data_merger => \&main::merge_trace,
	data_writer => \&DSC::extractor::write_data4,
      },
      dns_ip_version_vs_qtype => {
	keys2	=> [ qw(1 2 5 6 12 15 28 33 38 255 else) ],
	data_munger	=> \&main::munge_elsify,
	data_reader	=> \&DSC::extractor::read_data3,
	data_merger	=> \&main::merge_accum2d,
	data_writer	=> \&DSC::extractor::write_data3,
	#data_trimer	=> \&main::trim_accum2d,
      },
    },
  },
);

1;
