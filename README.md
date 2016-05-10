# DNS Statistics Collector

[![Build Status](https://travis-ci.org/DNS-OARC/dsc.svg?branch=develop)](https://travis-ci.org/DNS-OARC/dsc) [![Coverity Scan Build Status](https://scan.coverity.com/projects/8773/badge.svg)](https://scan.coverity.com/projects/dns-oarc-dsc)

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

# Install

Following build tools are needed, example for Debian/Ubuntu.

```
sudo apt-get install build-essential automake autoconf
```

Following dependencies are needed, example for Debian/Ubuntu.

```
sudo apt-get install libpcap-dev libproc-pid-file-perl
```

If you are installing from the GitHub repository you need to initialize the
submodule first and generate configure.

```
git submodule update --init
./autogen.sh
```

Now you can compile with optional options and install.

```
./configure [options ... ]
make
make install
```
