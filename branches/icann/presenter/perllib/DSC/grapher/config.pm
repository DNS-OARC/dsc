package DSC::grapher::config;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
		&read_config
        );
        %EXPORT_TAGS = ( );
        @EXPORT_OK   = qw();
}
use vars      @EXPORT;
use vars      @EXPORT_OK;

END { }

use strict;
use warnings;

my %CONFIG;

sub read_config {
	my $f = shift;
	open(F, $f) || die "$f: $!\n";
	while (<F>) {
		my @x = split;
		next unless @x;
		my $directive = shift @x;
		if ($directive eq 'server') {
			my $servername = shift @x;
			$CONFIG{servers}{$servername} = \@x;
		}
		if ($directive =~ /windows$/) {
			$CONFIG{$directive} = \@x;
		}
		if ($directive eq 'embargo') {
			$CONFIG{$directive} = $x[0];
		}
		if ($directive eq 'anonymize_ip') {
			$CONFIG{$directive} = 1;
		}
		if ($directive eq 'no_http_header') {
			$CONFIG{$directive} = 1;
		}
		if ($directive eq 'hide_nodes') {
			$CONFIG{$directive} = 1;
		}
		if ($directive eq 'timezone') {
			$ENV{TZ} = $x[0];
		}
		if ($directive eq 'domain_list') {
			my $listname = shift @x;
			die "Didn't find list-name after domain_list" unless defined($listname);
			push(@{$CONFIG{domain_list}{$listname}}, @x);
		}
		if ($directive eq 'valid_domains') {
			my $server = shift @x;
			die "Didn't find server-name after valid_domains" unless defined($server);
			my $listname = shift @x;
			die "domain list-name $listname does not exist"
				unless defined($CONFIG{domain_list}{$listname});
			$CONFIG{valid_domains}{$server} = $listname;
		}
	}
	close(F);
	\%CONFIG;
}

sub get_valid_domains {
	my $server = shift;
	print STDERR "get_valid_domains: server is $server\n";
	my $listname = $CONFIG{valid_domains}{$server};
	print STDERR "get_valid_domains: listname is $listname\n";
	return (undef) unless defined ($listname);
	return (undef) unless defined ($CONFIG{domain_list}{$listname});
	print STDERR "get_valid_domains: $server valid domain list is $listname\n";
	@{$CONFIG{domain_list}{$listname}};
}

1;
