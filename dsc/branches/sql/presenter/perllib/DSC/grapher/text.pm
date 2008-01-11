package DSC::grapher::text;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
                %TEXTS
        );
        %EXPORT_TAGS = ( );
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

use strict;
use warnings;

%TEXTS = (

  byserver => {
    description => '<p>The <strong>Queries by Server</strong> plot shows the
		number of queries sent to each server.',
  },

  bynode => {
    description => '<p>The <strong>Queries by Node</strong> plot shows the
		number of queries sent to each node.',
  },

  qtype => {
    description => '<p>The <strong>Queries by Qtype</strong> plot shows
		the breakdown of queries by DNS query type:</p>
		<ul>
		<li>A - Queries for the IPv4 address of a name.  Usually
		the most popular query type.
		<li>NS - Queries for the authoritative nameservers for
		a particular zone.  These are usually rare because end
		users (and their software agents) do not normally send
		NS queries.  NS records are normally included in the
		authority section of every DNS message.
		</ul>',
  },

  dnssec_qtype => {
    description => 'XX enter description here',
  },

  rcode => {
    description => 'XX enter description here',
  },

  opcode => {
    description => 'XX enter description here',
  },

  certain_qnames_vs_qtype => {
    description => 'XX enter description here',
  },

  client_subnet_accum => {
    description => 'XX enter description here',
  },

  client_subnet2_accum => {
    description => 'XX enter description here',
  },

  client_subnet2_trace => {
    description => 'XX enter description here',
  },

  client_subnet2_count => {
    description => 'XX enter description here',
  },

  qtype_vs_all_tld => {
    description => 'XX enter description here',
  },

  qtype_vs_invalid_tld => {
    description => 'XX enter description here',
  },

  qtype_vs_valid_tld => {
    description => 'XX enter description here',
  },

  qtype_vs_numeric_tld => {
    description => 'XX enter description here',
  },

  direction_vs_ipproto => {
    description => 'XX enter description here',
  },

  direction_vs_ipproto_sent => {
    description => 'XX enter description here',
  },

  query_attrs => {
    description => 'XX enter description here',
  },

  idn_qname => {
    description => 'XX enter description here',
  },

  rd_bit => {
    description => 'XX enter description here',
  },

  do_bit => {
    description => 'XX enter description here',
  },

  edns_version => {
    description => 'XX enter description here',
  },

  qtype_vs_qnamelen => {
    description => 'XX enter description here',
  },

 rcode_vs_replylen => {
    description => 'XX enter description here',
  },

  client_addr_vs_rcode_accum => {
    description => 'XX enter description here',
  },

  ipv6_rsn_abusers_accum => {
    description => 'XX enter description here',
  },

  chaos_types_and_names => {
    description => 'XX enter description here',
  },

);

1;
