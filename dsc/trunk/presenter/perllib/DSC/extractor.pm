use XML::Simple;
use POSIX;
use LockFile::Simple qw(lock trylock unlock);
use Data::Dumper;

package OARC::extractor;
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
		&grok_rcode_xml
		&grok_qtype_xml
		&grok_cltsub_xml
		&grok_tld_xml
		&grok_qtype_vs_qnamelen_xml
		&grok_rcode_vs_replylen_xml
		&grok_qtype_vs_tld_xml
		&grok_certain_qnames_vs_qtype_xml
		&grok_direction_vs_ipproto_xml
		$SKIPPED_KEY
        );
        %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }


# globals
$SKIPPED_KEY = "-:SKIPPED:-";	# must match dsc source code

my $lockfile_template = '/var/tmp/%F.lck';
my $LOCK_RETRY_DURATION = 45;

sub yymmdd {
	my $t = shift;
	my @t = gmtime($t);
	POSIX::strftime "%Y%m%d", @t;
}

sub read_data {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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
	#print "read $nl lines from $fn";
}

sub write_data {
	my $A = shift;
	my $fn = shift;
	my $nl = 0;
	my $B;
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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

# reads a 1-level hash database with no time dimension
# ie: key value
#
sub read_data2 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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
	#print "read $nl lines from $fn";
}


# writes a 1-level hash database with no time dimension
# ie: key value
#
sub write_data2 {
	my $A = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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
	#print "read $nl lines from $fn";
}

# writes a 2-level hash database with no time dimension
# ie: key1 key2 value
#
sub write_data3 {
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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
	#print "read $nl lines from $fn";
}

# writes a 2-level hash database WITH time dimension
# ie: time k1 (k:v:k:v) k2 (k:v:k:v)
#
sub write_data4 {  
	my $href = shift;
	my $fn = shift;
	my $nl = 0;
	my $lockmgr = LockFile::Simple->make(
		-format => $lockfile_template,
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

sub grok_rcode_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);

	my $href = $XML->{data}[0];
	my $aref = $href->{All};
	my @result;

	foreach my $x (@$aref) {
		#print "Rcode $x->{val}";
		my $rcode_aref = $x->{Rcode};
		foreach my $rcode_href (@$rcode_aref) {
			$result[$rcode_href->{val}] = $rcode_href->{count};
			#printf "Rcode %d total: %d\n", $x->{val}, $rcode_sum;
		}
	}
	($XML->{start_time}, @result);
}

sub grok_qtype_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);

	my $href = $XML->{data}[0];
	my $aref = $href->{All};
	my @result;

	foreach my $x (@$aref) {
		my $qtype_aref = $x->{Qtype};
		foreach my $qtype_href (@$qtype_aref) {
			$result[$qtype_href->{val}] = $qtype_href->{count};
		}
	}
	($XML->{start_time}, @result);
}

sub grok_cltsub_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my $n = 0;

	my $href = $XML->{data}[0];
	my $aref = $href->{All};
	my %result;

	foreach my $x (@$aref) {
		my $cltsub_aref = $x->{ClientSubnet};
		foreach my $cltsub_href (@$cltsub_aref) {
			$result{$cltsub_href->{val}} = $cltsub_href->{count};
			$n++;
		}
	}
	#print "grok_cltsub_xml: $n items in $fname";
	($XML->{start_time}, %result);
}

sub grok_tld_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my $n = 0;

	my $href = $XML->{data}[0];
	my $aref = $href->{All};
	my %result;

	foreach my $x (@$aref) {
		my $tld_aref = $x->{TLD};
		foreach my $tld_href (@$tld_aref) {
			$result{$tld_href->{val}} = $tld_href->{count};
			$n++;
		}
	}
	#print "grok_tld_xml: $n items in $fname";
	($XML->{start_time}, %result);
}

sub grok_qtype_vs_qnamelen_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my $n = 0;
	my %result;

	my $aref = $XML->{data}[0]->{Qtype};
	foreach my $qtypehref (@$aref) {
		my $qt = $qtypehref->{val};
		#print Dumper($qtypehref);
		foreach my $href (@{$qtypehref->{QnameLen}}) {
			my $ql = $href->{val};
			$result{$qt}->{$ql} = $href->{count};
			#print "  result{$qt}->{$ql} = $href->{count}";
		}
	}

	($XML->{start_time}, %result);
}

sub grok_rcode_vs_replylen_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	#print Data::Dumper($XML);
	my $n = 0;
	my %result;
	my $aref = $XML->{data}[0]->{Rcode};
	foreach my $rcodehref (@$aref) {
		my $rc = $rcodehref->{val};
		#print main::Dumper($rcodehref);
		foreach my $href (@{$rcodehref->{ReplyLen}}) {
			my $rl = $href->{val};
			$result{$rc}->{$rl} = $href->{count};
			#print "  result{$rc}->{$rl} = $href->{count}";
		}
	}
	($XML->{start_time}, %result);
}

sub grok_qtype_vs_tld_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my $n = 0;
	my %result;

	my $aref = $XML->{data}[0]->{Qtype};
	foreach my $qtypehref (@$aref) {
		my $qt = $qtypehref->{val};
		#print Dumper($qtypehref);
		foreach my $href (@{$qtypehref->{TLD}}) {
			next if defined ($href->{base64});
			my $ql = $href->{val};
			$result{$qt}->{$ql} = $href->{count};
			#print "  result{$qt}->{$ql} = $href->{count}";
		}
	}

	($XML->{start_time}, %result);
}

sub grok_certain_qnames_vs_qtype_xml {
	my $fname = shift || die;
	my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
	my $XML = $XS->XMLin($fname);
	my $n = 0;
	my %result;

	my $aref = $XML->{data}[0]->{CertainQnames};
	foreach my $qnamehref (@$aref) {
		my $qt = $qnamehref->{val};
		#print Dumper($qnamehref);
		foreach my $href (@{$qnamehref->{Qtype}}) {
			die if defined ($href->{base64});
			my $ql = $href->{val};
			$result{$qt}->{$ql} = $href->{count};
			#print "  result{$qt}->{$ql} = $href->{count}";
		}
	}

	($XML->{start_time}, \%result);
}


sub grok_direction_vs_ipproto_xml {
        my $fname = shift || die;
        my $XS = new XML::Simple(searchpath => '.', forcearray => 1);
        my $XML = $XS->XMLin($fname);
        my %result;

        my $href = $XML->{data}[0]->{Direction};
        foreach my $dirhash (@$href) {
                #print Dumper($dirhash);
                my $dir = $$dirhash{val};
                #print $dir;
                my $protocountarr = $$dirhash{IPProto};
                #print Dumper($protocountarr);
                foreach my $protocounthash (@$protocountarr) {
                        #print Dumper($protocounthash);
                        #print "result{$dir}{$$protocounthash{val}} = $$protocounthash{count}";
                        $result{$dir}{$$protocounthash{val}} = $$protocounthash{count};
                }
        }
        ($XML->{start_time}, \%result);
}

1;
