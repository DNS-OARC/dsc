package DSC::extractor;

use XML::Simple;
use POSIX;
use File::Flock;
use Digest::MD5;

use strict;

BEGIN {
	use Exporter   ();
	use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
	$VERSION     = 1.00;
	@ISA	 = qw(Exporter);
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
my $MD5_frequency = 500;

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

#
# time k1 v1 k2 v2 ...
#
sub read_data {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl, exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($k, %B) = split;
		$href->{$k} = \%B;
		$nl++;
	    }
	    close(IN);
	}
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
	my $lock = new File::Flock($fn);
	my $md = Digest::MD5->new;
	open(OUT, ">$fn.new") || die $!;
	foreach my $k (sort {$a <=> $b} keys %$A) {
		next unless defined($B = $A->{$k});
		my $line = join(' ', $k, %$B) . "\n";
		next unless ($line =~ /\S/);
		print OUT $line;
		$md->add($line);
		$nl++;
		print OUT "#MD5 ", $md->hexdigest, "\n" if (0 == ($nl % $MD5_frequency));
	}
	print OUT "#MD5 ", $md->hexdigest, "\n" if (0 != ($nl % $MD5_frequency));
	close(OUT);
	rename "$fn.new", $fn || die "$fn.new: $!";
	print "wrote $nl lines to $fn\n";
}

# a 1-level hash database with no time dimension
#
# ie: key value
#
sub read_data2 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl, exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($k, $v) = split;
		$href->{$k} = $v;
		$nl++;
	    }
	    close(IN);
	}
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
	my $lock = new File::Flock($fn);
	my $md = Digest::MD5->new;
	open(OUT, ">$fn.new") || die $!;
	foreach my $k (sort {$a cmp $b} keys %$A) {
		my $line = "$k $A->{$k}\n";
		print OUT $line;
		$md->add($line);
		$nl++;
		print OUT "#MD5 ", $md->hexdigest, "\n" if (0 == ($nl % $MD5_frequency));
	}
	print OUT "#MD5 ", $md->hexdigest, "\n" if (0 != ($nl % $MD5_frequency));
	close(OUT);
	rename "$fn.new", $fn || die "$fn.new: $!";
	print "wrote $nl lines to $fn\n";
}

# reads a 2-level hash database with no time dimension
# ie: key1 key2 value
#
sub read_data3 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl, exiting";
				return -1;
			}
			next;
		}
		$md->add($_);
		chomp;
		my ($k1, $k2, $v) = split;
		next unless defined($v);
		$href->{$k1}{$k2} = $v;
		$nl++;
	    }
	    close(IN);
	}
	$nl;
}

# writes a 2-level hash database with no time dimension
# ie: key1 key2 value
#
sub write_data3 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lock = new File::Flock($fn);
	my $md = Digest::MD5->new;
	open(OUT, ">$fn.new") || return;
	foreach my $k1 (keys %$href) {
		foreach my $k2 (keys %{$href->{$k1}}) {
			my $line = "$k1 $k2 $href->{$k1}{$k2}\n";
			print OUT $line;
			$md->add($line);
			$nl++;
			print OUT "#MD5 ", $md->hexdigest, "\n" if (0 == ($nl % $MD5_frequency));
		}
	}
	print OUT "#MD5 ", $md->hexdigest, "\n" if (0 != ($nl % $MD5_frequency));
	close(OUT);
	rename "$fn.new", $fn || die "$fn.new: $!";
	print "wrote $nl lines to $fn\n";
}


# reads a 2-level hash database WITH time dimension
# ie: time k1 (k:v:k:v) k2 (k:v:k:v)
#
sub read_data4 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $md = Digest::MD5->new;
	return 0 unless (-f $fn);
	if (open(IN, "$fn")) {
	    while (<IN>) {
		if (/^#MD5 (\S+)/) {
			if ($1 ne $md->hexdigest) {
				warn "MD5 checksum error in $fn at line $nl, exiting";
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
		$nl++;
	    }
	    close(IN);
	}
	$nl;
}

# writes a 2-level hash database WITH time dimension
# ie: time k1 (k:v:k:v) k2 (k:v:k:v)
#
sub write_data4 {  
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lock = new File::Flock($fn);
	my $md = Digest::MD5->new;
	open(OUT, ">$fn.new") || return;
	foreach my $ts (sort {$a <=> $b} keys %$href) {
		my @foo = ();
		foreach my $k1 (keys %{$href->{$ts}}) {
			push(@foo, $k1);
			push(@foo, join(':', %{$href->{$ts}{$k1}}));
		}
		my $line = join(' ', $ts, @foo) . "\n";
		print OUT $line;
		$md->add($line);
		$nl++;
		print OUT "#MD5 ", $md->hexdigest, "\n" if (0 == ($nl % $MD5_frequency));
	}
	print OUT "#MD5 ", $md->hexdigest, "\n" if (0 != ($nl % $MD5_frequency));
	close(OUT);
	rename "$fn.new", $fn || die "$fn.new: $!";
	print "wrote $nl lines to $fn\n";
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
