#!/usr/bin/perl
#
# Copyright (c) 2016, OARC, Inc.
# Copyright (c) 2007, The Measurement Factory, Inc.
# Copyright (c) 2007, Internet Systems Consortium, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

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

        		# Check if the file was previously linked but not yet unlinked
        		if ((stat($old))[1] != (stat($new))[1] ) {
        			link ($old, $new) or die "ln $old $new: $!";
        		}
		}

		unlink $old or die "unlink $old: $!";
	}
}
