dnl general stuff used by other checks
AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED(CONFIG_HOST_TYPE, "$host", "build environment")

AC_LANG([C++])


dnl save pre-set values (if any) for variables that are
dnl created by autoconf
PRESET_CXXFLAGS="$CXXFLAGS"
PRESET_LDFLAGS="$LDFLAGS"

dnl check for programs
AC_PROG_CXX
AC_PROG_INSTALL

# change default extention for source files
ac_ext=cc

AC_PROG_RANLIB
AC_PATH_PROG(AR, ar, ar)

AR_R="$AR r"
AC_SUBST(AR_R)

dnl check for optional features
AC_SUBST(std_include)
AC_ARG_ENABLE(std-include,
	[ --enable-std-include=DIR Where to find standard C++ headers ],
	[
		std_include=`echo -I$enableval | sed 's/:/ -I/g'`
		CPPFLAGS="$CPPFLAGS $std_include"
	],
	[ std_include='' ]
)

if test -z "$prefix" -o "$prefix" = "NONE"
then
	install_prefix="${ac_default_prefix}"
else
	install_prefix="${prefix}"
fi
echo "remembering installation prefix as $install_prefix"
AC_DEFINE_UNQUOTED(INSTALL_PREFIX, "$install_prefix", "installation prefix")



dnl adjust program options

OLD_CXXFLAGS=$CXXFLAGS
if test -z "$PRESET_CXXFLAGS"
then
	if test "x$GXX" = xyes
	then [

		# reset to preferred options
		# replace -O? with -O3
		CXXFLAGS=`echo $CXXFLAGS | sed 's/-O[0-9]*/-O3/'`;
		# enable useful warnings
		CXXFLAGS="$CXXFLAGS -Wall -Wwrite-strings -Woverloaded-virtual"
		# avoid disk I/O
		CXXFLAGS="$CXXFLAGS -pipe"

		# custom host-dependent tuning
		case "$host" in
		alpha-dec-osf4.*)
			# we get random coredumps with g++ -O
			echo deleting -On on $host
			CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-O[0-9]* *//'`
			;;
		*-linux-*)
			# -O2,3 seems to produce coredumps on RH and MDK linux
			echo enforcing -O1 on $host
			CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-O[0-9]* */-O1 /'`
			;;
		esac
	] fi
fi

if test "x$PRESET_CXXFLAGS" != "x$CXXFLAGS"
then
	AC_MSG_CHECKING(whether custom $CXX flags work)
	AC_TRY_COMPILE(
		[
		],[
			return 0;
		],[
			AC_MSG_RESULT(probably)
			echo "changing $CXX flags to $CXXFLAGS"
		],[
			AC_MSG_RESULT(no)
			CXXFLAGS=$OLD_CXXFLAGS
			echo "leaving  $CXX flags at $CXXFLAGS"
		]
	)
fi

dnl check for libraries
AC_CHECK_LIB(nsl, main)
AC_CHECK_LIB(socket, main)
AC_CHECK_LIB(m, main)

dnl check if compiler can find C++ headers
AC_CHECK_HEADER(iostream.h, [],
	AC_MSG_WARN([
		Failure to find iostream.h indicates
		a compiler installation problem; You 
		may want to use --enable-std-include 
		option to help your compiler to find
		directories with C++ include files.])
)

	
AC_PROG_MAKE_SET
