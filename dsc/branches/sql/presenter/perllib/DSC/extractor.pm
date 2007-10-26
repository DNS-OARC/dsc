package DSC::extractor;

use DBI;
use DSC::db;
use XML::Simple;
use POSIX;
use Digest::MD5;
#use File::Flock;
use File::NFSLock;
use Time::HiRes; # XXX for debugging

use strict;

BEGIN {
	use Exporter   ();
	use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
	$VERSION     = 1.00;
	@ISA	 = qw(Exporter);
	@EXPORT      = qw(
		&yymmdd
		&grok_1d_xml
		&grok_2d_xml
		&grok_array_xml
		&elsify_unwanted_keys
		&replace_keys
	);
	%EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
	@EXPORT_OK   = qw(
		$SKIPPED_KEY
		$SKIPPED_SUM_KEY
	);
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }


# globals
$SKIPPED_KEY = "-:SKIPPED:-";	# must match dsc source code
$SKIPPED_SUM_KEY = "-:SKIPPED_SUM:-";	# must match dsc source code

sub yymmdd {
	my $t = shift;
	my @t = gmtime($t);
	POSIX::strftime "%Y%m%d", @t;
}

# was used by old LockFile::Simple code
#
sub lockfile_format {
	my $fn = shift;
	my @x = stat ($fn);
	unless (defined ($x[0]) && defined($x[1])) {
		open(X, ">$fn");
		close(X);
	}
	@x = stat ($fn);
	die "$fn: $!" unless (defined ($x[0]) && defined($x[1]));
	'/tmp/' . join('.', $x[0], $x[1], 'lck');
}

sub lock_file {
	my $fn = shift;
#	return new File::Flock($fn);
	return File::NFSLock->new($fn, 'BLOCKING');
}

# read data in old flat file format (used by importer)
sub read_flat_data {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		$nl++;
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl\n".
					"found $1 expect ". $md->hexdigest. "\n".
					"exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($k, %B) = split;
		$href->{$k} = \%B;
	    }
	    close(IN);
	}
	$nl;
}

# read data in old flat file format (used by importer)
sub read_flat_data2 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		$nl++;
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl\n".
					"found $1 expect ". $md->hexdigest. "\n".
					"exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($k, $v) = split;
		$href->{$k} = $v;
	    }
	    close(IN);
	}
	$nl;
}

# read data in old flat file format (used by importer)
sub read_flat_data3 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		$nl++;
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl\n".
					"found $1 expect ". $md->hexdigest. "\n".
					"exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($k1, $k2, $v) = split;
		next unless defined($v);
		$href->{$k1}{$k2} = $v;
	    }
	    close(IN);
	}
	$nl;
}


# read data in old flat file format (used by importer)
sub read_flat_data4 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		$nl++;
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl\n".
					"found $1 expect ". $md->hexdigest. "\n".
					"exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($ts, %foo) = split;
		while (my ($k,$v) = each %foo) {
			my %bar = split(':', $v);
			$href->{$ts}{$k} = \%bar;
		}
	    }
	    close(IN);
	}
	$nl;
}

##############################################################################

sub grok_1d_xml {
	my $fname = shift || die "grok_1d_xml() expected fname";
	my $L2 = shift || die "grok_1d_xml() expected L2";
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my %result;
	my $aref = $XML->{data}[0]->{All};
	foreach my $k1ref (@$aref) {
		foreach my $k2ref (@{$k1ref->{$L2}}) {
			my $k2 = $k2ref->{val};
			$result{$k2} = $k2ref->{count};
		}
	}
	($XML->{start_time}, \%result);
}

sub grok_2d_xml {
	my $fname = shift || die;
	my $L1 = shift || die;
	my $L2 = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my %result;
	my $aref = $XML->{data}[0]->{$L1};
	foreach my $k1ref (@$aref) {
		my $k1 = $k1ref->{val};
		foreach my $k2ref (@{$k1ref->{$L2}}) {
			my $k2 = $k2ref->{val};
			$result{$k1}{$k2} = $k2ref->{count};
		}
	}
	($XML->{start_time}, \%result);
}

sub grok_array_xml {
	my $fname = shift || die;
	my $L2 = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my $aref = $XML->{data}[0]->{All};
	my @result;
	foreach my $k1ref (@$aref) {
		my $rcode_aref = $k1ref->{$L2};
		foreach my $k2ref (@$rcode_aref) {
			my $k2 = $k2ref->{val};
			$result[$k2] = $k2ref->{count};
		}
	}
	($XML->{start_time}, @result);
}

sub elsify_unwanted_keys {
	my $hashref = shift;
	my $keysref = shift;
	foreach my $k (keys %{$hashref}) {
		next if ('else' eq $k);
		next if (grep {$k eq $_} @$keysref);
		$hashref->{else} += $hashref->{$k};
		delete $hashref->{$k};
	}
}

sub replace_keys {
	my $oldhash = shift;
	my $oldkeys = shift;
	my $newkeys = shift;
	my @newkeycopy = @$newkeys;
	my %newhash = map { $_ => $oldhash->{shift @$oldkeys}} @newkeycopy;
	\%newhash;
}

##############################################################################

1;
