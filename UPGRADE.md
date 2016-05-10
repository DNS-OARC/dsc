# Upgrade

This document contains the upgrade information between the major versions
of DSC for the eventual breaking changes, please read CHANGES for the new
features added.

# From dsc-201502251630 to v2.0.0

The `dsc-201502251630` was the last version release before the use of
version numbering.

## Install Paths

Since the conform to FHS 3.0 paths have been changed but will only affect
new installations where `configure` is not touched.

In previous version `INSTALLDIR` was set to `/usr/local/dsc`, this is now
controlled by Automake and `configure` using `--prefix=DIR` or individual
path options as below.  See `configure --help` for information about default
paths and how to control each of them.

- `upload-*` scripts was previously installed in `$INSTALLDIR/libexec`,
  is now installed in `$libexecdir/dsc`.
- `dsc.conf.sample` was previously installed in `$INSTALLDIR/etc`, is now
  installed in `$sysconfdir/dsc`.
- `dsc` was previously installed in `$INSTALLDIR/bin`, is not installed
  in `$bindir`.

## Data Files Path

The path to `dsc` data files that it output has been changed to
`$localstatedir/lib/dsc` but only affects the path in `dsc.conf.sample`
during installation.  If you use an old configuration then `dsc` will
store the data files in the same path as before.  This can be controlled
by `configure --with-data-dir=DIR`.

## Configuration

Dataset names have been made unique so `dsc` will not start if there are
duplicates, you need to change the configuration so that all datasets
are unique.

The following indexers have been removed since they are only aliases:
- `cip4_addr`, use `client` instead.
- `cip4_net`, use `client_subnet` instead.
- `d0_bit`, use `do_bit` instead.

## Upload Scripts Deprecated

Altho the upload scripts are still installed they are now considered
deprecated and will be removed in future versions.

The uploads scripts where constructed for the purpose of uploading `dsc`
data to DNS-OARC and that is a very specific purpose that does not belong
in a software repository.  It can be replaced by instructions specific for
each organization that needs to do it.

## PID File

The PID file is now locked and `dsc` will not start if another process has
it locked.
