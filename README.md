# DNS Statistics Collector

[![Build Status](https://travis-ci.org/DNS-OARC/dsc.svg?branch=develop)](https://travis-ci.org/DNS-OARC/dsc) [![Coverity Scan Build Status](https://scan.coverity.com/projects/8773/badge.svg)](https://scan.coverity.com/projects/dns-oarc-dsc) [![Total alerts](https://img.shields.io/lgtm/alerts/g/DNS-OARC/dsc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/DNS-OARC/dsc/alerts/)

DNS Statistics Collector (DSC) is a tool used for collecting and exploring
statistics from busy DNS servers.  It uses a distributed architecture with
collectors running on or near nameservers sending their data to one or more
central presenters for display and archiving.  Collectors use pcap to sniff
network traffic.  They transmit aggregated data to the presenter as XML data.

DNS Statistics Presenter can be found here:
- https://github.com/DNS-OARC/dsp

More information about DSC/DSP may be found here:
- https://www.dns-oarc.net/tools/dsc
- https://www.dns-oarc.net/oarc/data/dsc

Issues should be reported here:
- https://github.com/DNS-OARC/dsc/issues

Mailinglist:
- https://lists.dns-oarc.net/mailman/listinfo/dsc

## Dependencies

`dsc` requires a couple of libraries beside a normal C compiling
environment with autoconf, automake and libtool.

`dsc` has a non-optional dependency on the PCAP library and optional
dependency on the GeoIP library (for the `asn` and `country` indexer).

To install the dependencies under Debian/Ubuntu:
```
apt-get install -y libpcap-dev libproc-pid-file-perl
```

To install the dependencies under CentOS (with EPEL enabled):
```
yum install -y libpcap-devel perl-Proc-PID-File
```

To install the dependencies under FreeBSD 10+ using `pkg`:
```
pkg install -y libpcap p5-Proc-PID-File
```

To install the dependencies under OpenBSD 5+ using `pkg_add`:
```
pkg_add p5-Proc-PID-File
```

NOTE: It is recommended to install the PCAP library from source/ports on
OpenBSD since the bundled version is an older and modified version.

## Building from source tarball

The source tarball from DNS-OARC comes prepared with `configure`:

```
tar zxvf dsc-version.tar.gz
cd dsc-version
./configure [options]
make
make install
```

## Building from Git repository

If you are building `dsc` from it's Git repository you will first need
to initiate the Git submodules that exists and later create autoconf/automake
files, this will require a build environment with autoconf, automake and
libtool to be installed.

```
git clone https://github.com/DNS-OARC/dsc.git
cd dsc
git submodule update --init
./autogen.sh
./configure [options]
make
make install
```

## Puppet

John Bond at ICANN DNS Engineering team has developed a puppet module for DSC,
the module and code can be found here:
- https://forge.puppet.com/icann/dsc
- https://github.com/icann-dns/puppet-dsc
