Name:           dsc
Version:        2.15.2
Release:        1%{?dist}
Summary:        DNS Statistics Collector
Group:          Productivity/Networking/DNS/Utilities

License:        BSD-3-Clause
URL:            https://www.dns-oarc.net/oarc/data/dsc
# Source needs to be generated by dist-tools/create-source-packages, see
# https://github.com/jelu/dist-tools
Source0:        https://www.dns-oarc.net/files/dsc/%{name}-%{version}.tar.gz?/%{name}_%{version}.orig.tar.gz

BuildRequires:  libpcap-devel
BuildRequires:  libmaxminddb-devel
%if 0%{?fedora}
BuildRequires:  GeoIP-devel
%endif
%if 0%{?centos} == 0 && 0%{?el7}
BuildRequires:  GeoIP-devel
%endif
%if 0%{?centos} == 0 && 0%{?el8}
BuildRequires:  GeoIP-devel
%endif
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  dnswire-devel >= 0.4.0
BuildRequires:  libuv-devel
BuildRequires:  python3
Requires:       python3

%description
DNS Statistics Collector (DSC) is a tool used for collecting and exploring
statistics from busy DNS servers. It can be set up to run on or near
nameservers to generate aggregated data that can then be transported to
central systems for processing, displaying and archiving.

Together with dsc-datatool the aggregated data can be furthur enriched
and converted for import into for example InfluxDB which can then be
accessed by Grafana for visualzation.


%prep
%setup -q -n %{name}_%{version}


%build
sh autogen.sh
%configure --enable-dnstap
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%config %{_sysconfdir}/*
%{_bindir}/*
%{_datadir}/doc/*
%{_mandir}/man1/*
%{_mandir}/man5/*


%changelog
* Fri Aug 30 2024 Jerry Lundström <lundstrom.jerry@gmail.com> 2.15.2-1
- Release 2.15.2
  * This releases fixes 3 issues detected by code analysis tools:
    - File not closed and memory not freed during error while loading known
      TLD file
    - https://github.com/DNS-OARC/dsc/security/code-scanning/22
      label buffer should be static
    - https://github.com/DNS-OARC/dsc/security/code-scanning/20
      unsigned difference expression
  * Commits:
    855f037 CodeQL
    b00cb11 Stack
    c4d3634 Sonar
    26c3b9e Badges, fixes
    b4a9171 Workflow
* Tue Apr 23 2024 Jerry Lundström <lundstrom.jerry@gmail.com> 2.15.1-1
- Release 2.15.1
  * This release fixes client subnet indexer which overwrote the mask
    options during initialization so the conf option `client_v4_mask` and
    `client_v6_mask` was never used.
  * Other changes:
    - Update documentation
    - Update builtin known TLDs based on PSL
    - Update copyright year
  * Commits:
    d577a97 Copyright
    f71edff Known TLDs
    dedafdd Client mask
    8ef947c Doc
* Wed Aug 09 2023 Jerry Lundström <lundstrom.jerry@gmail.com> 2.15.0-1
- Release 2.15.0
  * This release fixes DNS parsing w.r.t. EDNS, implements better loop
    detection during name decompression and adds a lot of EDNS indexers
    and filters.
  * Previously the DNS parser expected the additional records to come
    straight after the question section. Meaning that if the DNS packet
    had any answer or authority records, they would be parsed as additional
    records for the OPT record and EDNS information.
  * Following new indexers has been added:
    - edns_cookie
    - edns_cookie_len
    - edns_cookie_client
    - edns_cookie_server
    - edns_ecs
    - edns_ecs_family
    - edns_ecs_source_prefix
    - edns_ecs_scope_prefix
    - edns_ecs_address
    - edns_ecs_subnet
    - edns_ede
    - edns_ede_code
    - edns_ede_textlen
    - edns_ede_text
    - edns_nsid
    - edns_nsid_len
    - edns_nsid_data
    - edns_nsid_text
    Following new filters has been added:
    - edns0-only
    - edns0-cookie-only
    - edns0-nsid-only
    - edns0-ede-only
    - edns0-ecs-only
    See man-page dsc.conf(5) for more information.
  * Other fixes/additions:
    - Only parse entire DNS message if EDNS indexers are used
    - `dns_protocol`: Implement proper loop detection during decompression
    - `xmalloc`: Check return of `amalloc()` before using `memset()`/`memcpy()` because it's undefined behavior on null pointers
  * Commits:
    8259f30 EDNS filters
    41f3b9a strtohex, nsid text
    a666c04 EDNS(0) Client Subnet
    b5164fe EDNS
    7cabfd9 EDNS0 parsing fixes and additional EDNS0 indexers.
    46b1797 memcpy/memset fixes
    8fd7b7a EDNS parsing
    cee2bf7 EDNS0 parsing, multi RR test
    a2c00c9 DNS compression loop detection
    9875a3e RR parsing
* Thu Jun 15 2023 Jerry Lundström <lundstrom.jerry@gmail.com> 2.14.1-1
- Release 2.14.1
  * Fixed a bug in TLD handling when using `tld_list`, it did not reset
    where it was in the QNAME when nothing was found and could therefor
    wrongly indicate something as a TLD.
    Also fixed a typo in the `dsc.conf` man-page.
  * Commits:
    976589d GCOV
    c3afee4 TLD list, doc typo
* Mon Apr 03 2023 Jerry Lundström <lundstrom.jerry@gmail.com> 2.14.0-1
- Release 2.14.0
  * This release adds new configure option to control the file access to
    the output files, support for newer DNSTAP, improved DNSTAP message
    handling and updated Public Suffix List.
    - Fix #279: Add new conf options to control output file access:
      - `output_user`: set output file user ownership
      - `output_group`: set output file group ownership
      - `output_mod`: set output file mode bits
    - `dnstap`: Move DNSTAP essential attributes checks inside each type and customize them for that specific type
    - Update dnswire dependencies to v0.4.0
    - `encryption_index`: Add support for new DNSTAP DNS-over-QUIC socket protocol
    - Update builtin Public Suffix List (PSL)
  * Commits:
    abfe245 DNSTAP
    da06317 Output file access
    af01a48 DOQ transport, PSL update
* Fri Feb 10 2023 Jerry Lundström <lundstrom.jerry@gmail.com> 2.13.2-1
- Release 2.13.2
  * Updated pcap-thread to v4.0.1:
    Fixed issue with `pcap_dispatch()` during non-threaded timed runs by
    checking packet timestamp and use `pcap_breakloop()` if the run
    should end.
    Based on reports, it looks like `pcap_dispatch()` won't stop
    processing if load is high enough even if documentation says "only
    one bufferful of packets is read at a time".
  * Many thanks to Klaus Darilion @klaus3000 (NIC.AT) for the report
    and helping to track down the issue and test fixes!
  * Commits:
    e7d92fe Fix COPR
    7ecf217 pcap-thread
* Thu Apr 21 2022 Jerry Lundström <lundstrom.jerry@gmail.com> 2.13.1-1
- Release 2.13.1
  * This patch release is mainly for build and packages where MaxMind DB
    library is preferred over the legacy GeoIP library.
    MaxMind has announced that the databases for GeoIP will be EOL May 2022
    and recommends switching to GeoIP2 databases.
  * Also updated DSC's description, removing references to the now
    discontinued Presenter and pointing to dsc-datatool instead.
  * Commits:
    d891e2c Package, description
    c23406c Optional GeoIP
    26dd506 GeoIP
* Fri Jan 28 2022 Jerry Lundström <lundstrom.jerry@gmail.com> 2.13.0-1
- Release 2.13.0
  * This release fixes a huge performance issue with hashing IPv6
    addresses, adds support for new DNSTAP messages types and protocols,
    and adds two new indexers.
  * Thanks to a patch sent in by Ken Renard (@kdrenard) a rather huge
    performance issue related to hashing IPv6 addresses has been solved.
    Old code used a very incorrect assumption about addresses in general
    and while same way was used for IPv4, it didn't hit as hard as it did
    for IPv6.
    New code uses hashing functions on both address types and to quote
    the GitHub issue (by Ken):
    -"This performs about 5% better than what I did (51 sec versus 54 sec)
      for 5GB pcap file with nearly 50/50 split of IPv4 and IPv6 (3.7M/3.5M
      v4/v6 queries).
      Old inXaddr_hash() has been running for 75 minutes and is about 20%
      done. I say this is a winner!"
    Many thanks to Ken for pointing this out and supplying a patch!
  * DSC now depends on dnswire v0.3.0 which includes new DNSTAP messages
    types and protocols that was recently added to DNSTAP's Protobuf
    definition.
    The new `UPDATE_QUERY` and `UPDATE_RESPONSE` messages types are
    now supported and are interpret as `AUTH_QUERY` and `AUTH_RESPONSE`.
    The new socket protocols for DOT, DOH and DNSCrypt are also supported
    and are interpret as TCP for indexers such as `ip_proto` and
    `transport`. To get stats on the encryption itself you can use the
    new indexer `encryption`.
  * Two new indexers have been added:
    - `label_count`: Number of labels in the QNAME
    - `encryption`: Indicates whether the DNS message was carried over an
      encrypted connection or not, and if so over which. For example
      "unencrypted", "dot" (DNS-over-TLS), "doh" (DNS-over-HTTPS).
  * Other changes:
    - `inX_addr`: Rework structure, separate IPv4 and IPv6 addresses
    - Fix some DNSTAP tests
    - `transport_index`: Fix typo in code documentation
  * Commits:
    37df703 DNSTAP update, encryption indexer
    d27171f Label count indexer
    6932247 Adding labellen indexer which counts the number of labels in a DNS message
    68cc9c7 New IP hashing
* Thu Jan 13 2022 Jerry Lundström <lundstrom.jerry@gmail.com> 2.12.0-1
- Release 2.12.0
  * This release adds a new conf option `tld_list` to control what DSC
    considers are TLDs, and a script to convert the Public Suffix List to
    this format (see `man dsc-psl-convert` for more information).
  * For example, using this option will allow DSC to gather statistics on
    domains like `co.uk` and `net.au` that would otherwise be counted as
    `uk` and `au`.
  * The release also updates the man-pages, clarifying how to use multiple
    `interface` and other similar options. And removes the deprecated cron
    upload scripts.
  * Commits:
    e779a87 Remove upload scripts
    2880f93 PSL TLD list
    ea04022 Update Copyright and known TLDs
    5cbc7a4 Output format
    b7e6c35 Doc
    e66dae4 dh_auto_test
    6a3e817 debhelper
    89d033f Bye Travis
    fa1c179 Mattermost
* Tue Oct 20 2020 Jerry Lundström <lundstrom.jerry@gmail.com> 2.11.2-1
- Release 2.11.2
  * This release fixes a bug in `asn_indexer` that didn't enabled the usage
    of MaxMindDB after successful initiation. Other changes include a typo
    fix in `configure` and a lot of coverage tests.
  * Commits:
    395b11a Travis, configure
    ffea9ed Tests
    8b0bebd Tests
    09f8174 Config tests
    d1514d4 Coverage
    66b018c Coverage, ASN indexer
* Tue Aug 18 2020 Jerry Lundström <lundstrom.jerry@gmail.com> 2.11.1-1
- Release 2.11.1
  * This release fixes a 17-year old code cut&paste mistake in the
    classification indexer, until now it's been classifying funny query
    types based on the query class. This fix was sent in by Jim Hague
    (Sinodun), thanks Jim!
  * Other changes are based on code analysis reports and setup for code
    coverage.
  * Commits:
    8d4763c Correct funny-qtype classification.
    a1dd55e getline
    29bd143 Coverage
    685e504 SonarCloud
    f759515 Badges
* Mon Jun 01 2020 Jerry Lundström <lundstrom.jerry@gmail.com> 2.11.0-1
- Release 2.11.0
  * This release updates the built in known TLDs table and adds the optional
    configuration option `knowntlds_file` to, instead of using the built in
    table, load the data from a file.
  * If compiled with only MaxMindDB support then ASN and Country indexer
    would complain (and exit) that no database has been specified.
    This release changes the behavior to match that of GeoIP support,
    making it possible to run without specifying a database.
  * Other changes:
    - Fix compile warnings
    - COPR packaging fixes
    - `country_indexer`: Fixed typos in log messages (was copied from ASN)
    - Fix issues and false-positives reported by newer version of scan-build
  * Commits:
    e937d1 COPR
    1382370 country, asn
    423a813 scanbuild
    2571b97 Compile warnings
    4f69447 Known TLDs
* Thu May 07 2020 Jerry Lundström <lundstrom.jerry@gmail.com> 2.10.0-1
- Release 2.10.0
  * This release adds new configuration options to `dnstap_unixsock` to
    control ownership and permissions for the DNSTAP socket file.
  * Other fixes:
    - Unlink the DNSTAP socket file if an error during initialization occur
    - Do hard exit in forks to not run `atexit()` (which will unlink the
      DNSTAP socket file)
  * Commits:
    9d1d49a fork
    733b286 DNSTAP socket
* Thu Apr 02 2020 Jerry Lundström <lundstrom.jerry@gmail.com> 2.9.1-1
- Release 2.9.1
  * This release fixes a few bugs, removes a lot of the debug messages
    about DNSTAP and removes GeoIP from openSUSE/SLE packages as it has
    been deprecated on those platforms.
  * Changes:
    - `daemon`: Fix bug with listening for SIGINT when in foreground mode
    - `dnstap`:
      - Fix #217: Unlink UNIX socket on exit if successfully initiated
      - Fix startup bug, `exit()` if unable to initialize
      - Fix #220:
        - Remove/hide a lot of debug messages and the printing of the DNSTAP message
        - Clarify a lot of the info and error messages
        - Prefix all DNSTAP related messages with `DNSTAP: `
    - Fix compile warnings and include headers when GeoIP is missing
    - `asn_indexer`: Fix bug, said unknown IPv4 when it was IPv6
  * Commits:
    08bad5b DNSTAP debug
    1232264 LGTM
    589ea7a GeoIP, asn indexer
    4fea0d2 sigint, DNSTAP UNIX socket, DNSTAP init
* Fri Mar 20 2020 Jerry Lundström <lundstrom.jerry@gmail.com> 2.9.0-1
- Release 2.9.0
  * This release adds support for receiving DNS messages over DNSTAP along
    with documentation updates and eliminated compiler warnings.
  * To enable DNSTAP support, install dependencies (check `README.md`) and
    run configure with `--enable-dnstap`.
  * New configuration options:
    - `dnstap_file`: specify input from DNSTAP file
    - `dnstap_unixsock`: specify DNSTAP input from UNIX socket
    - `dnstap_tcp`: specify DNSTAP input from TCP connections (dsc listens)
    - `dnstap_udp`: specify DNSTAP input from UDP connections (dsc listens)
    - `dnstap_network`: specify network information in place of missing DNSTAP attributes
  * Other changes:
    - Add documentation about extra configure options that might be needed for FreeBSD/OpenBSD
    - Fix compile warnings on FreeBSD 11.2
    - Fix compile warning `snprintf()` truncation
    - Packaging updates
  * Commits:
    60e6950 DNSTAP
    af0417b README
    1f1b489 COPR, spec
    435e136 Package
    3f24feb FreeBSD 11 compatibility
    563b986 Funding
* Tue Apr 23 2019 Jerry Lundström <lundstrom.jerry@gmail.com> 2.8.1-1
- Release 2.8.1
  * Added all missing config options for the response time indexer:
    - `response_time_mode`
    - `response_time_bucket_size`
    - `response_time_max_queries`
    - `response_time_full_mode`
    - `response_time_max_seconds`
    - `response_time_max_sec_mode`
  * Commits:
    36f0280 Response time config
* Mon Feb 11 2019 Jerry Lundström <lundstrom.jerry@gmail.com> 2.8.0-1
- Release 2.8.0
  * This release brings an new indexer `response_time` (funded by NIC.AT!),
    support for MaxMind DB (GeoIP2) and an option to set the DNS port.
  * The new indexer `response_time` can track queries and report the time
    it took to receive the response in buckets of microseconds or in
    logarithmic scales (see `response_time_mode`). It will also report
    timeouts, missing queries (received a response but have never seen the
    query), dropped queries (due to memory limitations) and internal errors.
  * Here is an example output of log10 mode:
    <array name="response_time" dimensions="2" start_time="1478727151"
        stop_time="1478727180">
      <dimension number="1" type="All"/>
      <dimension number="2" type="ResponseTime"/>
      <data>
        <All val="ALL">
          <ResponseTime val="100000-1000000" count="77"/>
          <ResponseTime val="10000-100000" count="42"/>
          <ResponseTime val="1000-10000" count="3"/>
          <ResponseTime val="missing_queries" count="1"/>
        </All>
      </data>
    </array>
  * New configuration options:
    - `asn_indexer_backend`: Control what backend to use for the ASN indexer
    - `country_indexer_backend`: Control what backend to use for the
      country indexer
    - `maxminddb_asn`: Specify database for ASN lookups using MaxMind DB
    - `maxminddb_country`: Specify database for country lookups using
      MaxMind DB
    - `dns_port`: Control the DNS port
    - `response_time_mode`: Set the output mode of the response time indexer
    - `response_time_bucket_size`: The size of bucket (microseconds)
    - Following options exists to control internal aspects of `response_time`
      indexer, see man-page for more information:
      - `response_time_max_queries`
      - `response_time_full_mode`
      - `response_time_max_seconds`
      - `response_time_max_sec_mode`
  * Fixes:
    - Add LGTM and fix alerts
    - Update `pcap_layers` with fixes for `scan-build` warnings
    - Fix port in debug output of DNS message, was showing server port
      on responses
  * Commits:
    f38a655 License
    48cd44e Man-page, interface any, response time
    8b9345f LGTM Alert
    e57a013 DNS port
    38aa018 Response time statistics
    7a60d53 Cleanup
    5c45ce2 Copyright
    0dc8a3c MaxMind DB (GeoIP2)
    473387b LGTM, README, packages, scan-build
* Tue Aug 14 2018 Jerry Lundström <lundstrom.jerry@gmail.com> 2.7.0-1
- Release 2.7.0
  * Add support for Linux "cooked" capture encapsulation (`DLT_LINUX_SLL`).
  * Fixes:
    - `grok_question()`: Remove usage of `strcpy()`
    - `pcap_tcp_handler()`: Use `snprintf()`
    - `printable_dnsname()`: Use `snprintf()`
    - Fix CID 104450, 186871
  * Commits:
    41d59ac man-page HTML
    476d6ed pcap_layers, CID
    747131b Configure options
    43c9ad0 DLT_LINUX_SLL
    8a48667 Support the linux cooked sll frame
    bd4a94f Fix CID 104450
- change me
* Mon Aug 21 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 2.6.1-1
- Release 2.6.1
  * Compatibility fixes for FreeBSD 11.1+ which is now packing `struct ip`.
  * Commits:
    c0cd375 Handle compile warnings and FreeBSD's packing of structs
    c528ccb Code formatting and moved external code to own directory
* Tue Jul 11 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 2.6.0-1
- Release 2.6.0
  * Two new DNS filters and configuration for client subnet netmask has been
    added thanks to pull request submission from Manabu Sonoda (@mimuret), see
    `man 5 dsc.conf` for more details.
  * New DNS filters:
    - `servfail-only`: Count only SERVFAIL responses
    - `authentic-data-only`: Count only DNS messages with the AD bit is set
  * New configuration:
    - `client_v4_mask`: Set the IPv4 MASK for client_subnet INDEXERS
    - `client_v6_mask`: Set the IPv6 MASK for client_subnet INDEXERS
  * Fixes:
    - Set `_DEFAULT_SOURCE`, was giving compile warnings on some platforms
    - Update `pcap-thread` to v2.1.3 for compatibility fixes
    - Fix bug where extra `"` would be OK in configuration
    - Eat all white-space between tokens in configuration
    - Minor documentation corrections
  * Commits:
    8a20421 Config parse quote/whitespace bug
    4eb91d8 PR review and corrections
    1dcdbc1 add supports statistics for DNSSEC validation resolver - SERVFAIL
            DNS message filter - AD bit DNS message filter - set custom mask
            for ClientSubnet
    7c4ce7e Update pcap-thread to v2.1.3
    f5d152c Corrected date
    04f137d Prepare SPEC for OSB/COPR
    402c242 Config header is generated by autotools
* Wed Mar 29 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 2.5.1-1
- Release 2.5.1
  * Various compatibility issues and a possible runtime bug, related to
    pcap-thread, fixed.
  * Commits:
    5ed03e3 Compat for OS X
    8605759 Fix compiler warnings
    5fbad26 Update pcap-thread to v2.1.2
    47ed110 Update pcap-thread to v2.1.1
* Thu Mar 02 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 2.5.0-1
- Release 2.5.0
  * Resolved memory leaks within the IP fragment reassembly code that was
    reported by Klaus Darilion (NIC.AT) and added config option to control
    some parts of the fragment handling.
  * Fixes:
    - Add `pcap_layers_clear_fragments()` to remove old fragments after
      `MAX_FRAG_IDLE` (60 seconds)
    - Use correct alloc/free functions for dataset hash
    - Fix spacing in dsc.conf(5) man-page
  * New config option:
    - `drop_ip_fragments` will disable IP fragmentation reassembling and
      drop any IP packet that is a fragment (even the first)
  * Commits:
    eaee6c0 Drop IP fragments
    3ebb687 Issue #146: Fix leak in fragment handling
    9a5e377 Use correct alloc/free
    35f663c Fix #107: add const
* Fri Jan 27 2017 Jerry Lundström <lundstrom.jerry@gmail.com> 2.4.0-1
- Release 2.4.0
  * Since there have been a few major issues with the threaded capturing code
    it is now default disabled and have to be enabled with a configure option
    to use: `./configure --enable-threads ...`
  * A lot of work has been done to ensure stability and correct capturing,
    as of now `dsc` is continuously running on the testing platforms with
    simulated traffic and tests are performance every 5-15 minutes:
    - https://dev.dns-oarc.net/jenkins/view/dsctest/
  * With the rewrite of the config parser to C it was missed that Hapy allowed
    CR/LF within the values of the options.  Changing the C parser to allow
    it is a bit of work and having CR/LF within the value may lead to other
    issues so it is now documented that CR/LF are not allowed in config option
    values.
  * Fixes:
    - The `-T` flag was just controlling pcap-thread usage of threads, it now
      controls all usage of threads including how signals are caught.
    - Fix program name, was incorrectly set so it would be reported as `/dsc`.
    - Use thread safe functions (_r).
    - Handle very long config lines by not having a static buffer, instead
      let `getline()` allocate as needed.
    - Use new activation in pcap-thread to activate the capturing of pcaps
      after the initial interval sync have been done during start-up.
    - Use factions of second for start-up interval sync and interval wait.
    - Fix memory leaks if config options was specified more then once.
    - Use new absolute timed run in pcap-thread to more exactly end capturing
      at the interval.
    - Fix config parsing, was checking for tab when should look for line feed.
    - Exit correctly during pcap-thread run to honor `dump_reports_on_exit`.
    - Use 100ms as default pcap-thread timeout, was 1s before but the old code
      used 250ms.
    - Various enhancements to logging of errors.
  * New config options/features:
    - `pcap_buffer_size` can be used to increase the capture buffer within
      pcap-thread/libpcap, this can help mitigate dropped packets by the
      kernel during interval breaks.
    - `no_wait_interval` will skip the interval sync that happens during
      start-up and start capturing directly, the end of the interval will
      still be the modulus of the interval.
    - `pcap_thread_timeout` can be used to change the internal timeout use
      in pcap-thread to wait for packets (default 100ms).
    - Log non-fatal errors from pcap-thread w.r.t. setting the filter which
      can indicate that the filter is running in userland because lack of
      support or that it is too large for the kernel.
  * Special thanks to:
    - Anand Buddhdev, RIPE NCC
    - Klaus Darilion, NIC.AT
    - Vincent Charrade, Nameshield
  * Commits:
    ee59572 Fix #111, fix #116: Update pcap-thread to v2.0.0, remove debug
            code
    40a1fb4 Fix #139: Use 100ms as default pcap-thread timeout
    2a07185 Fix #137: Graceful exit on signal during run
    f1b3ec3 Issue #116: Try and make select issue more clear
    950ea96 Fix #133: Return from `Pcap_run()` on signal/errors
    667cc91 Issue #116: Add config option pcap_thread_timeout
    3c9e073 Notice if non-fatal errors was detected during activation
    4ea8f54 Fix #108: Document that CR/LF are not allowed within configuration
            line
    9fda332 Check for LF and not tab
    15a1dc0 Use pcap-thread timed run to interface
    1e98f8b Fix potential memory leaks if config options specified more then
            once
    a9b38e9 Add missing LF and indicate what config option was wrong if
            possible
    f8a2821 Use fractions of seconds for both start up interval sync and
            timed run, always adjust for inter-run processing delay
    f47069a Fix #121: Update to pcap-thread latest develop
    fc13d73 Issue #116: Feature for not waiting on the interval sync
    c832337 Fix #122: Update pcap-thread to v1.2.3 for fix in timed run
    4739111 Add `pcap_buffer_size` config option
    7d9bf90 Update pcap-thread to v1.2.2
    ef43335 Make threads optional and default disabled
    c2399cf getline() returns error on eof, don't report error if we are
    5c671e6 Clarify config error message and report `getline()` error
    8bd6a67 Fix #114: Handle very long lines
    47b1e1a Use _r thread safe functions when possible
    0f5d883 Update daemon.c
    f18e3ea Update doc, -T now disables all usage of threads
    57aacbe Honor the -T flag when installing signal handlers
* Thu Dec 22 2016 Jerry Lundström <lundstrom.jerry@gmail.com> 2.3.0-1
- Release 2.3.0
  * Rare lockup has been fixed that could happen if a signal was received
    in the wrong thread at the wrong time due to `pcap_thread_stop()`
    canceling and waiting on threads to join again. The handling of signals
    have been improved for threaded and non-threaded operations.
  * A couple of bugfixes, one to fix loading of GeoIP ASN database and
    another to use the lowest 32 bits of an IP address (being v4 or v6)
    in the IP hash making it a bit more efficient for v6 addresses.
  * New functionality for the configure option `local_address`, you can now
    specify a network mask (see `man 5 dsc.conf` for syntax).
  * Commits:
    e286298 Fix CID 158968 Bad bit shift operation
    c15db43 Update to pcap-thread v1.2.1
    1ac06ac Move stopping process to not require a packet
    597dd34 Handle signals better with and without pthreads
    bcf99e8 Add RPM spec and ACLOCAL_AMFLAGS to build on CentOS 6
    667fe69 fixed load geoIP ASN database from config-file
    e1304d4 Fix #97: Add optional mask to `local_address` so you can
            specify networks
    5dae7dd Fix #96: Hash the lowest 32 bits of IP addresses
* Tue Dec 13 2016 Jerry Lundström <lundstrom.jerry@gmail.com> 2.2.1-1
- Initial package
