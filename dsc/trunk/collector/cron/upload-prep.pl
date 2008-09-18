#!/usr/bin/perl

use Proc::PID::File;
use POSIX;

# PURPOSE:
#
# This script copies (links) XML output files from the DSC collector
# to one or more "transfer" directories.  This allows the XML files
# to be sent to multiple "presenter" boxes independently.  If one
# destination is unreachable, XML files accumulate there until
# it becomes available again.  Meanwhile, recent files are still
# sent to the other destinations.


exit 0 if Proc::PID::File->running(dir => '/tmp');

# wait a few seconds for 'dsc' to finish writing its XML files to disk
#
sleep 3;


foreach my $conf (</usr/local/dsc/etc/*.conf>) {
	next unless open (CONF, $conf);
	my $rundir = undef;
	while (<CONF>) {
		$rundir = $1 if (/^run_dir\s+"([^"]+)"/);
	}
	close(CONF);
	next unless $rundir;
	next unless chdir $rundir;

	
	while (<*.xml>) {
        	my $old = $_;
        	unless (/^(\d+)\.\w+\.xml$/) {
                	print "skipping $old\n";
                	next;
        	}
        	my $t = $1;
        	my $yymmdd = strftime('%Y-%m-%d', gmtime($t));

		foreach my $upload (<upload/*>) {
			unless (-d "$upload/$yymmdd") {
                		mkdir "$upload/$yymmdd"
					or die "mkdir $upload/$yymmdd: $!";
        		}
        		my $new = "$upload/$yymmdd/$old";
        		link ($old, $new) or die "ln $old $new: $!";
			#
			# the above link(2) could fail if the file was
			# previously linked but not yet unlinked.  To
			# workaround we could do some stat(2)s and
			# ignore if they have the same inum?
			#
		}

		unlink $old or die "unlink $old: $!";
	}
}
