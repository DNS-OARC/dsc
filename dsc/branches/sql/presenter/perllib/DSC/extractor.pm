package DSC::extractor;

use DBI;
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
		&get_dbh
		&get_server_id
		&get_node_id
		&table_exists
		&create_data_table
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
		$datasource
		$username
		$password
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
$datasource = undef;
$username = undef;
$password = undef;

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

sub get_dbh {
    # my $dbstart = Time::HiRes::gettimeofday;
    my $dbh = DBI->connect($datasource, $username, $password, {
	AutoCommit => 0
	}); # XXX
    if (!defined $dbh) {
	print STDERR "error connecting to database: $DBI::errstr\n";
	return undef;
    }
    # printf "opened db connection in %d ms\n",
    #     (Time::HiRes::gettimeofday - $dbstart) * 1000;
    return $dbh;
}

sub table_exists($$) {
    my ($dbh, $tabname) = @_;
    my $sth = $dbh->prepare_cached(
	"SELECT 1 FROM pg_tables WHERE tablename = ?");
    $sth->execute($tabname);
    my $result = scalar $sth->fetchrow_array;
    $sth->finish;
    return $result;
}

#
# Create a db table for a $nkeys-dimensional dataset
#
sub create_data_table {
    my ($dbh, $tabname, $nkeys) = @_;

    print "creating table $tabname\n";
    $dbh->do(
	"CREATE TABLE $tabname (" .
	"  server_id     SMALLINT NOT NULL, " .
	"  node_id       SMALLINT NOT NULL, " .
	"  start_time    INTEGER NOT NULL, " . # unix timestamp
	# "duration      INTEGER NOT NULL, " . # seconds
	(join '', map "key$_ VARCHAR NOT NULL, ", 1..$nkeys) .
	"  count         INTEGER NOT NULL " .
	## Omitting primary key and foreign keys improves performance of inserts	## without any real negative impacts.
	# "CONSTRAINT dsc_$tabname_pkey PRIMARY KEY (server_id, node_id, start_time, key1), " .
	# "CONSTRAINT dsc_$tabname_server_id_fkey FOREIGN KEY (server_id) " .
	# "    REFERENCES server (server_id), " .
	# "CONSTRAINT dsc_$tabname_node_id_fkey FOREIGN KEY (node_id) " .
	# "    REFERENCES node (node_id), " .
	")");

    # These indexes are needed for grapher performance.
    $dbh->do("CREATE INDEX ${tabname}_time ON $tabname(start_time)");
    $dbh->do("CREATE INDEX ${tabname}_key ON $tabname(" .
	(join ', ', map "key$_", 1..$nkeys) . ")");
}

sub get_server_id($$) {
    my ($dbh, $server) = @_;
    my $server_id;
    my $sth = $dbh->prepare("SELECT server_id FROM server WHERE name = ?");
    $sth->execute($server);
    my @row = $sth->fetchrow_array;
    if (@row) {
	$server_id = $row[0];
    } else {
	$dbh->do('INSERT INTO server (name) VALUES(?)', undef, $server);
	$server_id = $dbh->last_insert_id(undef, undef, 'server', 'server_id');
    }
    return $server_id;
}

sub get_node_id($$$) {
    my ($dbh, $server_id, $node) = @_;
    my $node_id;
    my $sth = $dbh->prepare(
	"SELECT node_id FROM node WHERE server_id = ? AND name = ?");
    $sth->execute($server_id, $node);
    my @row = $sth->fetchrow_array;
    if (@row) {
	$node_id = $row[0];
    } else {
	$dbh->do('INSERT INTO node (server_id, name) VALUES(?,?)',
	    undef, $server_id, $node);
	$node_id = $dbh->last_insert_id(undef, undef, 'node', 'node_id');
    }
    return $node_id;
}

#
# read $nkey-dimensional hash from db table
#
sub read_data_generic {
	my ($dbh, $href, $type, $server_id, $node_id, $first_time, $last_time, $nkeys, $withtime) = @_;
	my $nl = 0;
	my $tabname = "dsc_$type";
	my $sth;

	my @params = ($first_time);
	my $sql = "SELECT ";
	$sql .= "start_time, " if ($withtime);
	$sql .= join('', map("key$_, ", 1..$nkeys));
	$sql .= (!$withtime && defined $last_time) ? "SUM(count) " : "count ";
	$sql .= "FROM $tabname WHERE ";
	if (defined $last_time) {
	    $sql .= "start_time >= ? AND start_time < ? ";
	    push @params, $last_time;
	} else {
	    $sql .= "start_time = ? ";
	}
	$sql .= "AND server_id = ? ";
	push @params, $server_id;
	if (defined $node_id) {
	    $sql .= "AND node_id = ? ";
	    push @params, $node_id;
	}
	if (!$withtime && defined $last_time) {
	    $sql .= "GROUP BY " . join(', ', map("key$_", 1..$nkeys));
	}
	# print "SQL: $sql;  PARAMS: ", join(', ', @params), "\n";
	$sth = $dbh->prepare($sql);
	$sth->execute(@params);

	my $nkeycols = ($withtime ? 1 : 0) + $nkeys;
	while (my @row = $sth->fetchrow_array) {
	    $nl++;
	    if ($nkeycols == 1) {
		$href->{$row[0]} = $row[1];
	    } elsif ($nkeycols == 2) {
		$href->{$row[0]}{$row[1]} = $row[2];
	    } elsif ($nkeycols == 3) {
		$href->{$row[0]}{$row[1]}{$row[2]} = $row[3];
	    }
	}
	$dbh->commit;
	print "read $nl rows from $tabname\n";
	return $nl;
}

#
# read 1-dimensional hash with time from table with 1 minute buckets
#
sub read_data {
    return read_data_generic(@_, 1, 1);
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


#
# write 1-dimensional hash with time to table with 1 minute buckets
#
sub write_data {
	# parameter $t is ignored.
	my ($dbh, $A, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_$type";
	my $start = Time::HiRes::gettimeofday;
	my $nl = 0;
	my $rows;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $t (keys %$A) {
	    my $B = $A->{$t};
	    foreach my $k (keys %$B) {
		$dbh->pg_putline("$server_id\t$node_id\t$t\t$k\t$B->{$k}\n");
		$nl++;
	    }
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000;
}


# read 1-dimensional hash without time from table with 1 day buckets
#
sub read_data2 {
    return read_data_generic(@_, 1, 0);
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

# write 1-dimensional hash without time to table with 1 day buckets
#
sub write_data2 {
	my ($dbh, $href, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_$type";
	my $start = Time::HiRes::gettimeofday;
	my $nl = 0;
	my $rows;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $k1 (keys %$href) {
	    $dbh->pg_putline("$server_id\t$node_id\t$t\t$k1\t$href->{$k1}\n");
	    $nl++;
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000;
}

# read 2-dimensional hash without time from table with 1 day buckets
#
sub read_data3 {
    return read_data_generic(@_, 2, 0);
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

# write 2-dimensional hash without time to table with 1 day buckets
#
sub write_data3 {
	my ($dbh, $href, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_$type";
	my $start = Time::HiRes::gettimeofday;
	my $nl = 0;
	my $rows;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $k1 (keys %$href) {
		foreach my $k2 (keys %{$href->{$k1}}) {
		    $dbh->pg_putline("$server_id\t$node_id\t$t\t$k1\t$k2\t$href->{$k1}{$k2}\n");
		    $nl++;
		}
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000;
}


#
# read 2-dimensional hash with time from table with 1 minute buckets
#
sub read_data4 {
    return read_data_generic(@_, 2, 1);
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

#
# write 2-dimensional hash with time to table with 1 minute buckets
#
sub write_data4 {
	# parameter $t is ignored.
	my ($dbh, $A, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_$type";
	my $start = Time::HiRes::gettimeofday;
	my $nl = 0;
	my ($B, $C);
	my $rows;
	$dbh->do("COPY $tabname FROM STDIN");
	foreach my $t (keys %$A) {
	    $B = $A->{$t};
	    foreach my $k1 (keys %$B) {
		next unless defined($C = $B->{$k1});
		foreach my $k2 (keys %$C) {
		    $dbh->pg_putline("$server_id\t$node_id\t$t\t$k1\t$k2\t$C->{$k2}\n");
		    $nl++;
		}
	    }
	}
	$dbh->pg_endcopy;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000;
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
