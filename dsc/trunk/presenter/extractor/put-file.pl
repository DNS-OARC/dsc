#!/usr/bin/perl -w
# from http://www.apacheweek.com/features/put

use strict;
use warnings;

use Data::Dumper;
use POSIX;
use LockFile::Simple qw(lock trylock unlock);
use File::Temp qw(tempfile);
use Digest::MD5;

my $putlog;
my $TOPDIR;

my $filename;
my $tempname;
my $OUT;	# filehandle ref
my $clength;
my $method;
my $remaddr;
my $timestamp;
my $SERVER;
my $NODE;
my %MD5;
my $debug;

run();
exit 0;

sub run {
	$debug = 0;
	$putlog = "/usr/local/dsc/var/log/put-file.log";
	$TOPDIR = "/usr/local/dsc/data";

	$filename = '-';
	$tempname = '-';
	$clength = $ENV{CONTENT_LENGTH};
	$method = $ENV{REQUEST_METHOD} || '-';
	$remaddr = $ENV{REMOTE_ADDR} || '-';
	$timestamp = strftime("[%d/%b/%Y:%H:%M:%S %z]", localtime(time));
	$SERVER = get_envar(qw(SSL_CLIENT_S_DN_OU REDIRECT_SSL_CLIENT_OU));
	$NODE = get_envar(qw(SSL_CLIENT_S_DN_CN SSL_CLIENT_CN));
	%MD5 = ();

	umask 022;

	# Check we are using PUT method
	&reply(500, "No request method") unless defined ($method);
	&reply(500, "Request method is not PUT") if ($method ne "PUT");

	# Check we got some content
	&reply(500, "Content-Length missing or zero") if (!$clength);


	mkdir("$TOPDIR/$SERVER", 0700) unless (-d "$TOPDIR/$SERVER");
	mkdir("$TOPDIR/$SERVER/$NODE", 0700) unless (-d "$TOPDIR/$SERVER/$NODE");
	chdir "$TOPDIR/$SERVER/$NODE" || die "$TOPDIR/$SERVER/$NODE: $!";

	# Check we got a destination filename
	my $path = $ENV{PATH_TRANSLATED};
	&reply(500, "No PATH_TRANSLATED") if (!$path);
	my @F = split('/', $path);
	$filename = pop @F;
	($OUT, $tempname) = tempfile(
		TEMPLATE=>"put.XXXXXXXXXXXXXXXX",
		DIR=>'.',
		UNLINK=>0);

	&reply(409, "File Exists") if (-f $filename);

	# Read the content itself
	my $toread = $clength;
	my $content = "";
	while ($toread > 0)
	{
    	my $data;
    	my $nread = read(STDIN, $data, $toread);
    	&reply(500, "Error reading content") if !defined($nread);
    	$toread -= $nread;
    	$content .= $data;
	}
	
	print $OUT $content;
	close($OUT);
	
	if ($filename =~ /\.xml$/) {
		&reply(500, "$filename Exists") if (-f $filename);
		&reply(500, "rename $tempname $filename: $!") unless rename($tempname, $filename);
		chmod 0644, $filename;
		&reply(201, "Stored $filename\n");
	} elsif ($filename =~ /\.tar$/) {
		my $tar_output = '';
		print STDERR "running tar -xzvf $tempname\n" if ($debug);
		open(CMD, "tar -xzvf $tempname 2>&1 |") || die;
		#
		# gnutar prints extracted files on stdout, bsdtar prints
		# to stderr and adds "x" to beginning of each line.  F!
		#
		my @files;
		while (<CMD>) {
			chomp;
			my @x = split;
			my $f = pop(@x);
			push(@files, $f);
		}
		close(CMD);
		load_md5s();
		foreach my $f (@files) {
			next if ($f eq 'MD5s');
			if (check_md5($f)) {
				$tar_output .= "Stored $f\n";
			} else {
				unlink($f);
			}
		}
		close(CMD);
		unlink($tempname);
		&reply(201, $tar_output);
	} else {
		&reply(500, "unknown file type ($filename)");
	}
}

#
# Send back reply to client for a given status.
#

sub reply {
    my $status = shift;
    my $message = shift;
    my $logline;

    $clength = '-' unless defined($clength);
    $remaddr = sprintf "%-15s", $remaddr;
    $logline = "$remaddr - - $timestamp \"$method $TOPDIR/$SERVER/$NODE/$filename\" $status $clength";

    print "Status: $status\n";
    print "Content-Type: text/plain\n\n";

    if ($status >= 200 && $status < 300) {
	print $message;
    } else {
	print "Error Transferring File\n";
	print "An error occurred publishing this file: $message\n";
	$logline .= " ($message)" if defined($message);
	#print Dumper(\%ENV);
    }
    
    &log($logline);
    exit(0);
}

sub log {
	my $msg = shift;
	my $lockmgr = LockFile::Simple->make(
		-format => '/tmp/%F.lck',
		-max => 3,
		-delay => 1);
	my $lock = $lockmgr->lock($putlog) || return;
	if (open (LOG, ">> $putlog")) {
		print LOG "$msg\n";
		close(LOG);
	}
	$lock->release;
}

sub get_envar {
	my $val = undef;
	foreach my $name (@_) {
		last if defined($val = $ENV{$name});
	}
	&reply(500, 'No ' . join(' or ', @_)) unless defined($val);
	$val =~ tr/A-Z/a-z/;
	$val;
}

sub load_md5s {
	unless (open(M, "MD5s")) {
		warn "MD5s: $!";
		return;
	}
	while (my $line = <M>) {
		chomp $line;
		my ($hash, $fn) = split (/\s+/, $line);
		unless (defined($hash) && defined($fn)) {
			warn $line;
			next;
		}
		$MD5{$fn} = $hash;
		print STDERR "loaded $fn hash $hash\n" if ($debug);
	}
	close(M);
}

sub md5_file {
	my $fn = shift;
	my $ctx = Digest::MD5->new;
	open(F, $fn) || return "$!";
	$ctx->addfile(*F{IO});
	close(F);
	$ctx->hexdigest;
}

sub check_md5 {
	my $fn = shift;
	print STDERR "checking $fn\n" if ($debug);
	return 0 unless defined($MD5{$fn});
	my $file_hash = md5_file($fn);
	if ($MD5{$fn} eq $file_hash) {
		print STDERR "MD5s match!\n" if ($debug);
		return 1;
	}
	print STDERR "md5 mismatch for: $fn\n";
	print STDERR "orig hash = $MD5{$fn}\n";
	print STDERR "file hash = $file_hash\n";
	return 0;
}
