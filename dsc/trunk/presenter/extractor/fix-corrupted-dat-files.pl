#!/usr/bin/perl

use strict;
use warnings;
use Digest::MD5;

my $MD5_frequency = 500;

foreach my $fn (@ARGV) {
	do_file($fn);
}


sub do_file {
    my $fn = shift;
    open(I, $fn) or die "$fn: $!";
    open(O, ">$fn.fixed") or die "$fn.fixed: $!";
    my $nl = 0;
    my $md = Digest::MD5->new;
    my $buf = '';
    my $errs = 0;
    while (<I>) {
        $nl++;
	$buf .= $_;
        if (/^#MD5 (\S+)/) {
                if ($1 ne $md->hexdigest) {
                        warn "$fn: MD5 checksum error at line $nl\n";
                        $errs++;
                } else {
			#print "$fn: good up to line $nl\n";
			print O $buf;
		}
		$buf = '';
                next;
        }
        $md->add($_);
    }
    if ($errs == 0) {
	unlink("$fn.fixed");
#	print "$fn: okay\n";
    } else {
	# give fixed file the same uid:gid as old file
	my @sb = stat($fn);
	chown $sb[4], $sb[5], "$fn.fixed";
	rename ("$fn", "$fn.bad");
	rename ("$fn.fixed", "$fn");
	print "$fn: FIXED\n";
    }
}
