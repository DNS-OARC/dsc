dnl check if alleged C++ compiler understands C++
AC_MSG_CHECKING(whether the C++ compiler ($CXX) is a C++ compiler)
AC_TRY_COMPILE([
	#define Concat(a,b) a ## b
	struct T { 
		virtual ~T();
		virtual int m() const = 0;
		mutable bool a;
	};

	template <class P>
	struct C: public P {
		C(int): c(0) {}
		int m() const;
		int c;
	};

	template <class P>
	int C<P>::m() const { Concat(re,turn) c; }

	inline int test_inline() { return 0; }
],[
	// this is main()'s body
	static C<T> ct(1);
	return 0;
],[
	AC_MSG_RESULT(yes)
],[
	AC_MSG_RESULT(no)
	AC_MSG_ERROR(the compiler ($CXX) failed to pass a simple C++ test; check config.log for details)
])


dnl check for libraries
AC_CHECK_LIB(nsl, main)
AC_CHECK_LIB(socket, main)
AC_CHECK_LIB(m, main)

dnl checks for header files
AC_CHECK_HEADERS(\
	arpa/inet.h \
	fcntl.h \
	iomanip.h \
	iostream.h \
	math.h \
	ncurses.h \
	netdb.h \
	netinet/in.h \
	netinet/ip_dummynet.h \
	netinet/ip_fw.h \
	netinet/tcp.h \
	net/if.h \
	process.h \
	signal.h \
	string \
	string.h \
	strings.h \
	sstream \
	strstrea.h \
	strstream.h \
	sys/ioctl.h \
	sys/param.h \
	sys/resource.h \
	sys/select.h \
	sys/socket.h \
	sys/sockio.h \
	sys/sysinfo.h \
	sys/time.h \
	sys/types.h \
	time.h \
	unistd.h \
	regex.h \
	winbase.h \
	winsock2.h
)


dnl check for function parameters

AC_CACHE_CHECK(for signal handler type,
	ac_cv_signal_handler_type, [
		AC_TRY_COMPILE([
			#include <signal.h>
			extern void my_sig_handler(int signo);
		],[
			signal(SIGINT, my_sig_handler);
			return 0;
		],[
			ac_cv_signal_handler_type="void SignalHandler(int)";
		],[
			# best we can do without checking further
			ac_cv_signal_handler_type="void SignalHandler(...)";
		])
	]
)
AC_DEFINE_UNQUOTED(SIGNAL_HANDLER_TYPE, $ac_cv_signal_handler_type, "signal handler type")


dnl check for types

dnl check for rlim_t type in sys/socket.h
AC_CACHE_CHECK(for rlim_t, ac_cv_type_rlim_t, [
	AC_EGREP_CPP(
		[rlim_t[^a-zA-Z_0-9]],
		[
			#include <sys/types.h>
			#include <sys/resource.h>
			#if STDC_HEADERS
			#include <stdlib.h>
			#include <stddef.h>
			#endif
		],
		ac_cv_type_rlim_t=yes,
		ac_cv_type_rlim_t=no
	)
])
if test "x$ac_cv_type_rlim_t" = xyes; then
  AC_DEFINE(HAVE_TYPE_RLIM_T, 1, "rlim_t type")
fi

AC_CACHE_CHECK(for socklen_t, ac_cv_type_socklen_t, [
	AC_EGREP_CPP(
		[socklen_t[^a-zA-Z_0-9]],
		[
			#include <sys/types.h>
			#include <sys/socket.h>
			#if STDC_HEADERS
			#include <stdlib.h>
			#include <stddef.h>
			#endif
		],
		ac_cv_type_socklen_t=yes,
		ac_cv_type_socklen_t=no
	)
])
if test "x$ac_cv_type_socklen_t" = xyes; then
  AC_DEFINE(HAVE_TYPE_SOCKLEN_T, 1, "socklen_t type")
fi

AC_CACHE_CHECK(for rusage, ac_cv_have_type_rusage, [
	AC_TRY_COMPILE([
		#include <sys/time.h>
		#include <sys/resource.h>
	],[
		struct rusage R;
		return sizeof(R) == 0;
	],[
		ac_cv_have_type_rusage="yes"
	],[
		ac_cv_have_type_rusage="no"
	])
])
if test "x$ac_cv_have_type_rusage" = xyes ; then
  AC_DEFINE(HAVE_TYPE_RUSAGE, 1, "rusage type")
fi


dnl tm.tm_gmtoff
AC_CACHE_CHECK(for timeval, ac_cv_have_type_timeval, [
	AC_TRY_COMPILE([
		#include <time.h>
		#include <sys/time.h>
	],[
		struct timeval t;
		t.tv_sec = 0;
		t.tv_usec = 0;
		return 0;
	],[
		ac_cv_have_type_timeval="yes";
	],[
		ac_cv_have_type_timeval="no";
	])
])
if test "x$ac_cv_have_type_timeval" = xyes; then
	AC_DEFINE(HAVE_TYPE_TIMEVAL, 1, "timeval type")
fi


AC_CACHE_CHECK(for ios::fmtflags, ac_cv_have_type_iosfmtflags, [
	AC_TRY_COMPILE([
		#include <iostream.h>
	],[
		ios::fmtflags flags;
		return sizeof(flags) == 0;
	],[
		ac_cv_have_type_iosfmtflags="yes"
	],[
		ac_cv_have_type_iosfmtflags="no"
	])
])
if test "x$ac_cv_have_type_iosfmtflags" = xyes ; then
  AC_DEFINE(HAVE_TYPE_IOSFMTFLAGS, 1, "ios::fmtflags type")
fi


dnl sockaddr.sa_len
AC_CACHE_CHECK(whether sockaddr has sa_len,
	ac_cv_sockaddr_has_sa_len, [
		AC_TRY_COMPILE([
			#include <sys/types.h>
			#include <sys/socket.h>
		],[
			// this is main()'s body
			struct sockaddr addr;
			addr.sa_len = 0;
			return 0;
		],[
			ac_cv_sockaddr_has_sa_len="yes";
		],[
			ac_cv_sockaddr_has_sa_len="no";
		])
	]
)	
if test "x$ac_cv_sockaddr_has_sa_len" = xyes; then
	AC_DEFINE(HAVE_SA_LEN, 1, "sockaddr.addr member")
fi


dnl tm.tm_gmtoff
AC_CACHE_CHECK(whether tm has tm_gmtoff,
	ac_cv_tm_has_tm_gmtoff, [
		AC_TRY_COMPILE([
			#include <time.h>
			#include <sys/time.h>
		],[
			struct tm t;
			t.tm_gmtoff = 0;
			return 0;
		],[
			ac_cv_tm_has_tm_gmtoff="yes";
		],[
			ac_cv_tm_has_tm_gmtoff="no";
		])
	]
)
if test "x$ac_cv_tm_has_tm_gmtoff" = xyes; then
	AC_DEFINE(HAVE_TM_GMTOFF, 1, "tm.tm_gmtoff member")
fi


dnl check for global variables

dnl timezone(s)
AC_DEFUN(AC_TMP, [AC_TRY_COMPILE([
	#include <time.h>
	#include <sys/time.h>
],[
	return (int)_timezone;
],[
	ac_cv_var_timezone="_timezone";
],[
	AC_TRY_COMPILE([
		#include <time.h>
		#include <sys/time.h>
		extern time_t timezone;
	],[
		return (int)timezone;
	],[
		ac_cv_var_timezone="extern";
	],[
		ac_cv_var_timezone="none";
	])
])])


AC_CACHE_CHECK(for global timezone variable,
	ac_cv_var_timezone, [
		AC_TRY_RUN([
			#include <time.h>
			#include <sys/time.h>
			
			int main() {
				/* function name or a variable name? */
				return (((void*)&timezone) == ((void*)timezone)) ? -1 : 0;
			}
		],[
			ac_cv_var_timezone="timezone";
		],[
			AC_TMP
		],[
			AC_TMP
		])
	]
)

if test "x$ac_cv_var_timezone" = xnone; then
	:;
else
	if test "x$ac_cv_var_timezone" = xextern; then
		AC_DEFINE(HAVE_EXTERN_TIMEZONE, 1, "timezone variable")
		AC_DEFINE(HAVE_TIMEZONE, timezone, "timezone symbol")
	else
		AC_DEFINE_UNQUOTED(HAVE_TIMEZONE, $ac_cv_var_timezone, "timezone symbol")
	fi
fi

AC_CACHE_CHECK(for altzone,
	ac_cv_have_altzone, [
		AC_TRY_COMPILE([
			#include <time.h>
			#include <sys/time.h>
		],[
			return (int)altzone;
		],[
			ac_cv_have_altzone="yes";
		],[
			ac_cv_have_altzone="no";
		])
	]
)
if test "x$ac_cv_have_altzone" = xyes; then
	AC_DEFINE(HAVE_ALTZONE, 1, "altzone variable")
fi


dnl check for compiler characteristics

dnl check for functions and methods
AC_CHECK_FUNCS(\
	rint \
	ceilf \
	gettimeofday \
	getpagesize \
	getrlimit \
	getrusage \
	ioctl \
	signal \
	unlink \
	random \
	srandomdev \
	sleep \
	fork \
	strcasecmp \
	strncasecmp \
	timegm \
	pclose \
	popen \
	inet_makeaddr \
	inet_lnaof \
	inet_netof \
	\
	ioctlsocket \
	stricmp \
	strnicmp \
	closesocket
)

AC_CACHE_CHECK(for set_new_handler, ac_cv_have_set_new_handler, [
	AC_TRY_RUN([
			#include <new>
			static void myHandler() {}
			int main() {
				set_new_handler(&myHandler);
				return 0;
			}
		],
		ac_cv_have_set_new_handler="yes",
		ac_cv_have_set_new_handler="no",
		ac_cv_have_set_new_handler="no",
	)
])
if test "x$ac_cv_have_set_new_handler" = xyes ; then
  AC_DEFINE(HAVE_SET_NEW_HANDLER, 1, "set_new_handler function")
fi

dnl keep this check in sync with sstream.h
AC_CACHE_CHECK(for sstream::freeze, ac_cv_have_sstreamfreeze, [
	AC_TRY_COMPILE([
#if defined(HAVE_SSTREAM)
#   include <sstream>
#else
#	if defined(HAVE_STRSTREAM_H)
#		include <strstream.h>
#	elif defined(HAVE_STRSTREA_H) /* MS file name limit */
#		include <strstrea.h>
#	endif

#	include <string>

	class istringstream: public istrstream {
		public:
			istringstream(const string &buf):
				istrstream(buf.data(), buf.size()) {}
	};

	class ostringstream: public ostrstream {
		public:
			string str() const { return string(((ostrstream*)this)->str()); }
	};
#endif
	],[
		ostringstream os;
		os.freeze(false);
		return sizeof(os) == 0;
	],[
		ac_cv_have_sstreamfreeze="yes"
	],[
		ac_cv_have_sstreamfreeze="no"
	])
])
if test "x$ac_cv_have_sstreamfreeze" = xyes ; then
  AC_DEFINE(HAVE_SSTREAMFREEZE, 1, "sstream.freeze member function")
fi


dnl check for system services


AC_MSG_CHECKING(Default FD_SETSIZE value)
AC_TRY_RUN([
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/types.h>
	int main() {
		fprintf(fopen("conftestval", "w"), "%d\n", FD_SETSIZE);
		return 0;
	}
	],
	DEFAULT_FD_SETSIZE=`cat conftestval`,
	DEFAULT_FD_SETSIZE=-1,
	DEFAULT_FD_SETSIZE=-1,
)
AC_MSG_RESULT($DEFAULT_FD_SETSIZE)
AC_DEFINE_UNQUOTED(DEFAULT_FD_SETSIZE, $DEFAULT_FD_SETSIZE, "default FD_SETSIZE value")


AC_MSG_CHECKING(Maximum number of filedescriptors we can open)
AC_TRY_RUN(
	[
		/* this ingenuous check is derived from uncopyrighted Squid/configure.in */
		#include <stdio.h>
		#include <unistd.h>
		
		#include <sys/time.h>	/* needed on FreeBSD */
		#include <sys/param.h>
		#include <sys/resource.h>

		// see SSI_FD_NEWMAX below
		#ifdef HAVE_SYS_SYSINFO_H
		#include <sys/sysinfo.h>
		#endif
		
		int main() {
		#ifdef SSI_FD_NEWMAX
			if (setsysinfo(SSI_FD_NEWMAX, 0, 0, 0, 1) != 0)
				perror("setsysinfo(SSI_FD_NEWMAX)");
		#endif

		#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
		#if !defined(RLIMIT_NOFILE)
		#define RLIMIT_NOFILE RLIMIT_OFILE
		#endif
			struct rlimit rl;
			if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
				perror("getrlimit: RLIMIT_NOFILE");
			} else {
			rl.rlim_cur = rl.rlim_max; /* set it to the max */
			if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
				perror("setrlimit: RLIMIT_NOFILE");
			}
		#endif /* RLIMIT_NOFILE || RLIMIT_OFILE */

			/* by starting at 2^15, we will never exceed 2^16 */
			int i,j;
			i = j = 1<<15;
			while (j) {
				j >>= 1;
				if (dup2(0, i) < 0) { 
					i -= j;
				} else {
					close(i);
					i += j;
				}
			}
			i++;
			FILE *fp = fopen("conftestval", "w");
			fprintf (fp, "%d\n", i);
			return 0;
		}
	],
	PROBED_MAXFD=`cat conftestval`,
	PROBED_MAXFD=-1,
	PROBED_MAXFD=-2
)
AC_MSG_RESULT($PROBED_MAXFD)
AC_DEFINE_UNQUOTED(PROBED_MAXFD, $PROBED_MAXFD,
	"probed maximum number of file descriptors")

