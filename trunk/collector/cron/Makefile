
SCRIPTS += upload-x509.sh
SCRIPTS += upload-rsync.sh
SCRIPTS += upload-ssh.sh
SCRIPTS += upload-prep.pl

INSTALLDIR=/usr/local/dsc

all:
	@if perl -e 'use Proc::PID::File;' 2>/dev/null ; then \
		true ; \
	else \
		echo "WARNING:" ; \
		echo "    upload-prep.pl requires the Proc::PID::File Perl module" ; \
		echo "    Please install it before proceeding" ; \
		false ; \
	fi

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	test -d $(INSTALLDIR)/libexec || mkdir $(INSTALLDIR)/libexec
	install -m 755 $(SCRIPTS) $(INSTALLDIR)/libexec/

clean:
