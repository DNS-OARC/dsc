# DNS Statistics Collector

[![Total alerts](https://img.shields.io/lgtm/alerts/g/DNS-OARC/dsc.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/DNS-OARC/dsc/alerts/) [![Bugs](https://sonarcloud.io/api/project_badges/measure?project=dns-oarc%3Adsc&metric=bugs)](https://sonarcloud.io/dashboard?id=dns-oarc%3Adsc) [![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=dns-oarc%3Adsc&metric=security_rating)](https://sonarcloud.io/dashboard?id=dns-oarc%3Adsc)

DNS Statistics Collector (DSC) is a tool used for collecting and exploring
statistics from busy DNS servers. It can be set up to run on or near
nameservers to generate aggregated data that can then be transported to
central systems for processing, displaying and archiving.

Together with `dsc-datatool` the aggregated data can be furthur enriched
and converted for import into for example InfluxDB which can then be
accessed by Grafana for visualzation, see this wiki on how to set up that:
- https://github.com/DNS-OARC/dsc-datatool/wiki/Setting-up-a-test-Grafana

DSC data transforming and enriching tool can be found here:
- https://github.com/DNS-OARC/dsc-datatool

More information about DSC may be found here:
- https://www.dns-oarc.net/tools/dsc
- https://www.dns-oarc.net/oarc/data/dsc

Issues should be reported here:
- https://github.com/DNS-OARC/dsc/issues

General support and discussion:
- Mattermost: https://chat.dns-oarc.net/community/channels/oarc-software
- mailing-list: https://lists.dns-oarc.net/mailman/listinfo/dsc

## Dependencies

`dsc` requires a couple of libraries beside a normal C compiling
environment with autoconf, automake, libtool and pkgconfig.

`dsc` has a non-optional dependency on the PCAP library and optional
dependency on the MaxMindDB library (for the `asn` and `country` indexer).

To install the dependencies under Debian/Ubuntu:
```
apt-get install -y libpcap-dev
```

To install the dependencies under CentOS (with EPEL enabled):
```
yum install -y libpcap-devel
```

To install the dependencies under FreeBSD 10+ using `pkg`:
```
pkg install -y libpcap
```

NOTE: It is recommended to install the PCAP library from source/ports on
OpenBSD since the bundled version is an older and modified version.

### DNSTAP support

To enable DNSTAP support, first install the necessary dependencies and
then run `configure` with `--enable-dnstap`.

- Debian/Ubuntu: `apt-get install -y libdnswire-dev libuv1-dev`
- CentOS: `yum install -y dnswire-devel libuv-devel`
- FreeBSD: `pkg install -y libuv`
- OpenBSD: `pkg_add libuv`

`dnswire` packages for Debian, Ubuntu and CentOS exists at
[https://dev.dns-oarc.net/packages/](https://dev.dns-oarc.net/packages/),
for other distributions please see
[https://github.com/DNS-OARC/dnswire](https://github.com/DNS-OARC/dnswire).

## Building from source tarball

The [source tarball from DNS-OARC](https://www.dns-oarc.net/dsc/download)
comes prepared with `configure`:

```
tar zxvf dsc-version.tar.gz
cd dsc-version
./configure [options]
make
make install
```

NOTE: If building fails on FreeBSD/OpenBSD, try adding these configure
options: `--with-extra-cflags="-I /usr/local/include" --with-extra-ldflags="-L/usr/local/lib"`.

## Building from Git repository

If you are building `dsc` from it's Git repository you will first need
to initiate the Git submodules that exists and later create autoconf/automake
files, this will require a build environment with autoconf, automake, libtool
and pkgconfig to be installed.

```
git clone https://github.com/DNS-OARC/dsc.git
cd dsc
git submodule update --init
./autogen.sh
./configure [options]
make
make install
```

NOTE: If building fails on FreeBSD/OpenBSD, try adding these configure
options: `--with-extra-cflags="-I /usr/local/include" --with-extra-ldflags="-L/usr/local/lib"`.

## Puppet

John Bond at ICANN DNS Engineering team has developed a puppet module for DSC,
the module and code can be found here:
- https://forge.puppet.com/icann/dsc
- https://github.com/icann-dns/puppet-dsc
