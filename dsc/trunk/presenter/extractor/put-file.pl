#!/usr/bin/perl -w
# from http://www.apacheweek.com/features/put

use Data::Dumper;
use POSIX;
use LockFile::Simple qw(lock trylock unlock);
use File::Temp qw(tempfile);

my $putlog = "/usr/local/dsc/var/log/put-file.log";
my $TOPDIR = "/usr/local/dsc/data";
my $SERVER = undef;
my $NODE = undef;

my $filename = '-';
my $tempname = '-';
my $clength = $ENV{CONTENT_LENGTH} || '-';
my $method = $ENV{REQUEST_METHOD} || '-';
my $remaddr = $ENV{REMOTE_ADDR} || '-';
my $timestamp = strftime("[%d/%b/%Y:%H:%M:%S %z]", localtime(time));

umask 022;


# Check we are using PUT method
&reply(500, "No request method") unless defined ($method);
&reply(500, "Request method is not PUT") if ($method ne "PUT");

# Check we got some content
&reply(500, "Content-Length missing or zero") if (!$clength);

&reply(500, "No SSL_CLIENT_S_DN_OU")
	unless defined ($SERVER = $ENV{SSL_CLIENT_S_DN_OU});
&reply(500, "No SSL_CLIENT_S_DN_CN")
	unless defined ($NODE = $ENV{SSL_CLIENT_S_DN_CN});
$SERVER =~ tr/A-Z/a-z/;
$NODE =~ tr/A-Z/a-z/;
mkdir("$TOPDIR/$SERVER", 0700) unless (-d "$TOPDIR/$SERVER");
mkdir("$TOPDIR/$SERVER/$NODE", 0700) unless (-d "$TOPDIR/$SERVER/$NODE");
chdir "$TOPDIR/$SERVER/$NODE" || die;

# Check we got a destination filename
my $path = $ENV{PATH_TRANSLATED};
&reply(500, "No PATH_TRANSLATED") if (!$path);
my @F = split('/', $path);
$filename = pop @F;
($OUT, $tempname) = tempfile(TEMPLATE=>"put.XXXXXXXXXXXXXXXX",
	DIR=>'.',
	UNLINK=>0);

&reply(409, "File Exists") if (-f $filename);

# Read the content itself
$toread = $clength;
$content = "";
while ($toread > 0)
{
    $nread = read(STDIN, $data, $toread);
    &reply(500, "Error reading content") if !defined($nread);
    $toread -= $nread;
    $content .= $data;
}

# Write it out 
# Note: doesn't check the location of the file, whether it already
# exists, whether it is a special file, directory or link. Does not
# set the access permissions. Does not handle subdirectories that
# need creating.
#&reply(500, "Cannot write to $tempname") unless open(OUT, "> $tempname");
print $OUT $content;
close($OUT);

if ($filename =~ /\.xml$/) {
	&reply(500, "$filename Exists") if (-f $filename);
	&reply(500, "rename $tempname $filename: $!") unless rename($tempname, $filename);
	chmod 0644, $filename;
	&reply(201, "Stored $filename\n");
} elsif ($filename =~ /\.tar$/) {
	my $tar_output = '';
	open(CMD, "tar -xzvf $tempname 2>&1 |") || die;
	#
	# gnutar prints extracted files on stdout, bsdtar prints
	# to stderr and adds "x" to beginning of each line.  F!
	#
	while (<CMD>) {
		my @x = split;
		my $f = pop(@x);
		$tar_output .= "Stored $f\n";
		#print STDERR "Stored $f";
	}
	close(CMD);
	unlink($tempname);
	&reply(201, $tar_output);
} else {
	&reply(500, "unknown file type ($filename)");
}

exit(0);

#
# Send back reply to client for a given status.
#

sub reply
{
    my $status = shift;
    my $message = shift;
    my $logline;

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

sub log
{
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
