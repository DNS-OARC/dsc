package DSC::grapher::config;

BEGIN {
        use Exporter   ();
        use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);
        $VERSION     = 1.00;
        @ISA         = qw(Exporter);
        @EXPORT      = qw(
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
		if ($directive eq 'timezone') {
			$ENV{TZ} = $x[0];
		}
	}
	close(F);
	\%CONFIG;
}

1;
