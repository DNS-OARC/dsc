package OARC::extractor;

use XML::Simple;
use POSIX;
use LockFile::Simple qw(lock trylock unlock);
use Data::Dumper;

use strict;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
		&yymmdd
		&read_data
		&write_data
		&read_data2
		&write_data2
		&read_data3
		&write_data3
		&read_data4
		&write_data4
		&grok_1d_xml
		&grok_2d_xml
		&grok_array_xml
		&elsify_unwanted_keys
		&replace_keys
		$SKIPPED_KEY
		$SKIPPED_SUM_KEY
        );
        %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }


# globals
$SKIPPED_KEY = "-:SKIPPED:-";	# must match dsc source code
$SKIPPED_SUM_KEY = "-:SKIPPED_SUM:-";	# must match dsc source code

#my $lockfile_template = '/tmp/%F.lck';
#my $lockfile_template = '%f.lck';
my $LOCK_RETRY_DURATION = 45;

sub yymmdd {
	my $t = shift;
	my @t = gmtime($t);
	POSIX::strftime "%Y%m%d", @t;
}

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

#
# time k1 v1 k2 v2 ...
#
sub read_data {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	return unless (-f $fn);
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	if (open(IN, "$fn")) {
	    while (<IN>) {
		my ($k, %B) = split;
		$$href{$k} = \%B;
		$nl++;
	    }
	    close(IN);
	}
	$lock->release;
	$nl;
}

#
# time k1 v1 k2 v2 ...
#
sub write_data {
	my $A = shift;
	my $fn = shift;
	my $nl = 0;
	my $B;
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	open(OUT, ">$fn") || die $!;
	foreach my $k (sort {$a <=> $b} keys %$A) {
		$B = $$A{$k};
		print OUT join(' ', $k, %$B);
		$nl++;
	}
	close(OUT);
	$lock->release;
	print "wrote $nl lines to $fn";
}

# a 1-level hash database with no time dimension
#
# ie: key value
#
sub read_data2 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	return unless (-f $fn);
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	if (open(IN, "$fn")) {
	    while (<IN>) {
		my ($k, $v) = split;
		$$href{$k} = $v;
		$nl++;
	    }
	    close(IN);
	}
	$lock->release;
	$nl;
}


# a 1-level hash database with no time dimension
#
# ie: key value
#
sub write_data2 {
	my $A = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	open(OUT, ">$fn") || die $!;
	foreach my $k (sort {$a cmp $b} keys %$A) {
		print OUT "$k $$A{$k}";
		$nl++;
	}
	close(OUT);
	$lock->release;
	print "wrote $nl lines to $fn";
}

# reads a 2-level hash database with no time dimension
# ie: key1 key2 value
#
sub read_data3 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	return unless (-f $fn);
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	if (open(IN, "$fn")) {
	    while (<IN>) {
		my ($k1, $k2, $v) = split;
		next unless defined($v);
		$$href{$k1}->{$k2} = $v;
		$nl++;
	    }
	    close(IN);
	}
	$lock->release;
	$nl;
}

# writes a 2-level hash database with no time dimension
# ie: key1 key2 value
#
sub write_data3 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	open(OUT, ">$fn") || return;
	foreach my $k1 (keys %$href) {
		foreach my $k2 (keys %{$href->{$k1}}) {
			print OUT "$k1 $k2 $href->{$k1}->{$k2}";
			$nl++;
		}
	}
	close(OUT);
	$lock->release;
	print "wrote $nl lines to $fn";
}


# reads a 2-level hash database WITH time dimension
# ie: time k1 (k:v:k:v) k2 (k:v:k:v)
#
sub read_data4 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	return unless (-f $fn);
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION,
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	if (open(IN, "$fn")) {
	    while (<IN>) {
		my ($ts, %foo) = split;
		while (my ($k,$v) = each %foo) {
			my %bar = split(':', $v);
			$$href{$ts}{$k} = \%bar;
		}
		$nl++;
	    }
	    close(IN);
	}
	$lock->release;
	$nl;
}

# writes a 2-level hash database WITH time dimension
# ie: time k1 (k:v:k:v) k2 (k:v:k:v)
#
sub write_data4 {  
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => &lockfile_format($fn),
		-max => $LOCK_RETRY_DURATION, 
		-delay => 1);
	my $lock = $lockmgr->lock($fn) || die "could not lock $fn";
	open(OUT, ">$fn") || return;
	foreach my $ts (sort {$a <=> $b} keys %$href) {
		my @foo = ();
		foreach my $k1 (keys %{$$href{$ts}}) {
			push(@foo, $k1);
			push(@foo, join(':', %{$$href{$ts}{$k1}}));
		}
		print OUT join(' ', $ts, @foo);
		$nl++;
	}
	close(OUT);
	$lock->release;
	print "wrote $nl lines to $fn";
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
			$result{$k1}->{$k2} = $k2ref->{count};
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
                $$hashref{else} += $$hashref{$k};
                delete $$hashref{$k};
        }
}

sub replace_keys {
        my $oldhash = shift;
	my $oldkeys = shift;
	my $newkeys = shift;
	my @newkeycopy = @$newkeys;
	my %newhash = map { $_ => $$oldhash{shift @$oldkeys}} @newkeycopy;
	\%newhash;
}

##############################################################################
