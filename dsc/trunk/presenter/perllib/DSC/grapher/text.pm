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

  bynode => {
    description => '<p>The <strong>Queries by Node</strong> plot shows the
		amount of queries coming from each node in the server
		cluster.  If you would like to see the traffic for
		a single node, select the node name in the Servers/Nodes
		menu on the left.</p>
		<p>Note that the <em>By Node</em> option disappears from the
		Plots list when you are viewing the data for a single node.
		It reappears if you click on the Server name in the
		Servers/Nodes menu.</p>',
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
	</ul>
        <p>Click on the legend to view the queries for a specific type.</p>',
  },

  dnssec_qtype => {
    description => '<p>The <strong>DNSSEC Query Types</strong> plot shows
	queries recieved for certain DNSSEC-related query types (shown
	in the legend).  The first-generation DNSSEC types
	(SIG, KEY, NXT) have been obsoleted by newer, second-generation
	types (DS, RRSIG, NSEC, DNSKEY).
        <p>Click on the legend to view the queries for a specific type.</p>',
  },

  rcode => {
    description => '<p>The <strong>Replies by Rcode</strong> plot shows
	the breakdown of RCODE values in DNS responses:</p>
	<ul>
	<li>NOERROR - No error condition.
	<li>SERVFAIL - The name server was unable to process the
	query due to a problem with the name server.
	<li>NXDOMAIN - The domain name referenced in the query does
	not exist.
	<li>REFUSED - The server refuses to perform the specified
	operation for policy reasons.
	<li>NXRRSET - Some RRset that ought to exist, does not exist (used
	in dynamic updates).
	<li>Other - All other response codes, including: FORMERR, NOTIMP, YXDOMAIN, YXRRSET, NOTAUTH, NOTZONE.
	</ul>
        <p>Click on the legend to view the queries for a specific RCODE.</p>',
  },

  opcode => {
    description => '<p>The <strong>Messages by Opcode</strong> plot shows
	the breakdown of OPCODE values, other than QUERY, in DNS queries.
	Since QUERY is the most common opcode, its inclusion here would	
	dominate the plot.  Some of the less common opcodes are:</p>
	<ul>
	<li>IQUERY - An "inverse query" which has been deprecated
	in favor of the PTR query type.
	<li>STATUS - A server status request.
	<li>NOTIFY - Allows master servers to inform slave servers when a zone has changed.
	<li>UPDATE - Add or delete RRs or RRsets from a zone.
	<li>Other - All other opcode values, legitimate or not.
	</ul>
        <p>Click on the legend to view the queries for a specific opcode.</p>',
  },

  certain_qnames_vs_qtype => {
    description => '<p>This plot shows
	queries of type A, AAAA, or A6 for the name <strong>localhost</strong>
	and anything under <strong>root-servers.net</strong>.  We chose these
	names because, historically, the DNS root servers see a significant
	number of queries for them.  These plots may be uninteresting on
	normal nameservers.</p>
	<p>Click on the legend to view the queries for a specific type/name
	pair</p>',
	
  },

  client_subnet_accum => {
    description => '<p>The <strong>Busiest Client Subnet</strong> plot shows
	the number of queries sent from each /24 network.  The horizontal
	bars are colored based on the RIR where the address space is
	registered, followed by a two-letter country code.
	</p>',
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
