package DSC::db;

use DBI;
use POSIX;
use File::Temp qw(tempfile);
use Time::HiRes; # XXX for debugging

use strict;

BEGIN {
	use Exporter   ();
	use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
	$VERSION     = 1.00;
	@ISA	 = qw(Exporter);
	@EXPORT      = qw();
	%EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],
	@EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

$DSC::db::specific->{mysql} = {

id_type => 'TINYINT', # smaller than SMALLINT
autoinc => 'AUTO_INCREMENT',

from_dummy => sub {
    "FROM DUAL";
},

table_exists => sub {
    my ($dbh, $tabname) = @_;

    my $sth = $dbh->prepare_cached(
	"SELECT 1 FROM INFORMATION_SCHEMA.TABLES " .
	"WHERE table_schema = 'dsc' AND table_name = ?");
    $sth->execute($tabname);
    my $result = scalar $sth->fetchrow_array;
    $sth->finish;
    return $result;
},

# returns a reference to an array of data table names
data_table_names => sub {
    my ($dbh) = @_;
    return $dbh->selectcol_arrayref(
	"SELECT table_name FROM INFORMATION_SCHEMA.VIEWS " .
	"WHERE table_schema = 'dsc' AND table_name LIKE 'dsc_%'");
},

specific_init_db => sub {
    my ($dbh) = @_;
    # This flag is needed to detect missing InnoDB support.
    $dbh->do("SET SESSION sql_mode='NO_ENGINE_SUBSTITUTION'");
    $dbh->do("ALTER TABLE server ENGINE=INNODB");
    $dbh->do("ALTER TABLE node ENGINE=INNODB");
    $dbh->do("ALTER TABLE loaded_files ENGINE=INNODB");
},

create_data_table => sub {
    my ($dbh, $tabname) = @_;
    # This flag is needed to detect missing InnoDB support.
    $dbh->do("SET SESSION sql_mode='NO_ENGINE_SUBSTITUTION'");
    &{$DSC::db::default{create_data_table}}(@_);
    $dbh->do("ALTER TABLE ${tabname}_old ENGINE=INNODB");
    $dbh->do("ALTER TABLE ${tabname}_new ENGINE=INNODB");
},

# returns a reference to an array of data table index names
data_index_names => sub {
    my ($dbh) = @_;
    return $dbh->selectcol_arrayref(
	"SELECT DISTINCT index_name FROM INFORMATION_SCHEMA.STATISTICS " .
	"WHERE table_schema = 'dsc' AND index_name LIKE 'dsc_%'");
},

# returns an index hint, because mysql sometimes chooses badly
read_data_post_table => sub {
    my ($tabname) = @_;
    return $tabname =~ /_old$/ ? " /*! USE INDEX(${tabname}_time) */" : '';
},

#
# write 1-dimensional hash with time to table with 1 minute buckets
#
write_data => sub {
	# parameter $t is ignored.
	my ($dbh, $A, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	my ($fh, $fname) = tempfile("/tmp/dsc.XXXXXXXX");
	foreach my $t (keys %$A) {
	    my $B = $A->{$t};
	    foreach my $k (keys %$B) {
		print $fh "$t\t$k\t$B->{$k}\n";
		$nl++;
	    }
	}
	close $fh;
	$dbh->do("LOAD DATA LOCAL INFILE '$fname' " .
	    "INTO TABLE $tabname (start_time, key1, count) " .
	    "SET server_id = $server_id, node_id = $node_id");
	unlink $fname;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},

# write 1-dimensional hash without time to table with 1 day buckets
#
write_data2 => sub {
	my ($dbh, $href, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	my ($fh, $fname) = tempfile("/tmp/dsc.XXXXXXXX");
	foreach my $k1 (keys %$href) {
	    print $fh "$k1\t$href->{$k1}\n";
	    $nl++;
	}
	close $fh;
	$dbh->do("LOAD DATA LOCAL INFILE '$fname' " .
	    "INTO TABLE $tabname (key1, count) " .
	    "SET start_time = $t, server_id = $server_id, node_id = $node_id");
	unlink $fname;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},

# write 2-dimensional hash without time to table with 1 day buckets
#
write_data3 => sub {
	my ($dbh, $href, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	my ($fh, $fname) = tempfile("/tmp/dsc.XXXXXXXX");
	foreach my $k1 (keys %$href) {
	    foreach my $k2 (keys %{$href->{$k1}}) {
		print $fh "$k1\t$k2\t$href->{$k1}{$k2}\n";
		$nl++;
	    }
	}
	close $fh;
	$dbh->do("LOAD DATA LOCAL INFILE '$fname' " .
	    "INTO TABLE $tabname (key1, key2, count) " .
	    "SET start_time = $t, server_id = $server_id, node_id = $node_id");
	unlink $fname;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},

#
# write 2-dimensional hash with time to table with 1 minute buckets
#
write_data4 => sub {
	# parameter $t is ignored.
	my ($dbh, $A, $type, $server_id, $node_id, $t) = @_;
	my $tabname = "dsc_${type}";
	my $start = Time::HiRes::gettimeofday if $main::perfdbg;
	my $nl = 0;
	my ($B, $C);
	my ($fh, $fname) = tempfile("/tmp/dsc.XXXXXXXX");
	foreach my $t (keys %$A) {
	    $B = $A->{$t};
	    foreach my $k1 (keys %$B) {
		next unless defined($C = $B->{$k1});
		foreach my $k2 (keys %$C) {
		    print $fh "$t\t$k1\t$k2\t$C->{$k2}\n";
		    $nl++;
		}
	    }
	}
	close $fh;
	$dbh->do("LOAD DATA LOCAL INFILE '$fname' " .
	    "INTO TABLE $tabname (start_time, key1, key2, count) " .
	    "SET server_id = $server_id, node_id = $node_id");
	unlink $fname;
	printf "wrote $nl rows to $tabname in %d ms\n",
	    (Time::HiRes::gettimeofday - $start) * 1000 if $main::perfdbg;
},
};

1;
