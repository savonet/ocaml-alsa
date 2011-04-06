
# check for one particular file of the sources 
AC_INIT(ocaml-alsa, 0.2.1, savonet-users@lists.sourceforge.net)

VERSION=$PACKAGE_VERSION
AC_MSG_RESULT([configuring $PACKAGE_STRING])

# Add prefix to compilation variables
# if passed
if test "x$prefix" != "xNONE"; then
	CFLAGS="$CFLAGS -I$prefix/include"
	LDFLAGS="$LDFLAGS -L$prefix/lib"
	CPPFLAGS="$CPPFLAGS -I$prefix/include"
fi

AC_CHECK_OCAML_COMPILERS()

AC_PROG_CC()
AC_CHECK_TOOL([AR],[ar],no)
PKG_PROG_PKG_CONFIG()
#PKG_CHECK_MODULES(libasound2, alsa, , [AC_MSG_ERROR(libasound2 not found.)])

# PKG_CHECK_MODULES loses when you need --libs-only-[lL]
if ! $PKG_CONFIG --exists alsa; then
        AC_MSG_ERROR([libasound2 not found])
fi
libasound2_CFLAGS=`$PKG_CONFIG --cflags alsa`
AC_SUBST([libasound2_CFLAGS])
libasound2_LIBS=`$PKG_CONFIG --libs-only-l alsa`
AC_SUBST([libasound2_LIBS])
libasound2_LDFLAGS=`$PKG_CONFIG --libs-only-L alsa`
AC_SUBST([libasound2_LDFLAGS])

# substitutions to perform
AC_SUBST(VERSION)

# Finally create the Makefile and samples
AC_CONFIG_FILES([Makefile],[chmod a-w Makefile])
AC_CONFIG_FILES([src/META])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT
