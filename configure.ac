dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)

dnl Name your plug-in here
m4_define([plugin_name], [refocus-it])

dnl These two define the plug-in version number
m4_define([plugin_major_version], [2])
m4_define([plugin_minor_version], [0])

m4_define([plugin_version], [plugin_major_version.plugin_minor_version])
m4_define([plugin_package_name], [gimp-refocus-it])
m4_define([plugin_package_home], [https://github.com/JoesCat/gimp-refocus-it])
m4_define([plugin_package_email], [https://github.com/JoesCat/gimp-refocus-it/issues])

AC_INIT([plugin_name], [plugin_version], [plugin_package_email],
	[plugin_package_name], [plugin_package_home])

AC_DEFINE(PLUGIN_NAME, ["plugin_name"], [Plug-In name])
AC_DEFINE(PLUGIN_EMAIL, [plugin_package_email], [Plug-In package email])
AC_DEFINE(PACKAGE_NAME, ["plugin_package_name"], [Plug-In Package name])
AC_DEFINE(PLUGIN_VERSION, ["plugin_version"], [Plug-In version])

AC_DEFINE(PLUGIN_MAJOR_VERSION, plugin_major_version, [Plug-In major version])
AC_DEFINE(PLUGIN_MINOR_VERSION, plugin_minor_version, [Plug-In minor version])

AC_CONFIG_SRCDIR([src/image.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AC_CANONICAL_HOST
AC_CANONICAL_BUILD
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE([foreign -Wall])
#--------------------------------------------------------------------------
# automake 1.12 needs AM_PROG_AR but automake < 1.11.2 doesn't recognize it
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_INIT
AC_SUBST([LIBTOOL_DEPS])

#--------------------------------------------------------------------------
# Checks for programs.
AC_PROG_CC
AC_PROG_SED
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PATH_PROG([CHMOD],[chmod],[:])
AC_PATH_PROG([STRIP],[strip],[:])
AC_PATH_PROG([GIMP],[gimp-2.10],[:])
AC_PATH_PROG([GIMPTOOL],[gimptool-2.0],[:])
AC_PATH_PROG([MD5SUM],[md5sum],[:])
AC_PATH_PROG([MSGFMT],[msgfmt],[:])
AC_PATH_PROG([MSGINIT],[msginit],[:])
AC_PATH_PROG([MSGMERGE],[msgmerge],[:])
AC_PATH_PROG([XGETTEXT],[xgettext],[:])
AM_CONFIG_HEADER(refocus-it-config.h)
AC_PROG_INSTALL
AC_PROG_MAKE_SET

ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AM_MAINTAINER_MODE

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

#--------------------------------------------------------------------------
AC_SEARCH_LIBS([strerror],[cposix])

# Search for math.h include and math lib (some OSes have -lm built-in).
have_libm=maybe
AC_CHECK_HEADERS([math.h],
    AC_SEARCH_LIBS([cos],[m],[have_libm=yes]))
if test x"${have_libm}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the Math library and math.h],[1])
fi

dnl GIMP PLUG-IN
AC_ARG_WITH([gimp], AS_HELP_STRING([--with-gimp],[Install GIMP plug-in version of refocus-it @<:@default=yes@:>]), with_gimp=$withval, with_gimp=yes)
AM_CONDITIONAL(REFOCUS_GIMP, test x$with_gimp = xyes)

dnl DEBUG
AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug],[Enable debugging @<:@default=no@:>]), , enable_debug=no)

dnl DOUBLES
AC_ARG_ENABLE([doubles], AS_HELP_STRING([--enable-doubles],[Use doubles instead of floats @<:@default=no@:>]), enable_doubles=$enableval, enable_doubles=no)

if test x$enable_debug = xno ; then
	AC_DEFINE(NDEBUG,, Enable debugging)
fi

if test x$enable_doubles = xyes ; then
	AC_DEFINE(USE_DOUBLES,, Use doubles instead of floats)
fi

dnl i18n stuff
LOCALEDIR='${datadir}/locale'
AC_SUBST(LOCALEDIR)

# Avoid being locked to a particular gettext verion, use what's available.
have_gettext=no
AC_CHECK_HEADERS([intl.h],[have_gettext=yes])
AC_CHECK_FUNC([gettext],[have_gettext=yes],[have_gettext=no])
AC_CHECK_HEADERS([locale.h],,[have_gettext=no])
if test x"${have_gettext}" = xno; then
  AC_SEARCH_LIBS([intl],[have_gettext=yes],[
  AC_MSG_ERROR([ERROR: gettext() required! Please install libintl or GETTEXT Packages.])])
fi
AC_CHECK_FUNC([bind_textdomain_codeset],,[have_gettext=no])
AC_CHECK_FUNC([textdomain],,[have_gettext=no])
if test x"${have_gettext}" = xno; then
  AC_SEARCH_LIBS([intl],[have_gettext=yes],[
  AC_MSG_ERROR([ERROR: gettext() required! Please install locale, textdomain related Packages.])])
fi
GETTEXT_PACKAGE=gimp20-refocus-it
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext translation domain.])
if test x"${have_gettext}" = xyes; then
  AC_DEFINE([HAVE_GETTEXT],1,[Enable use of local languages])
fi
SNIPPET2='
gimplocaledir = $(localedir)
ifeq ($(shell id -u),0)
  gimplocaledir = $(LOCALEDIR)
endif
'
AC_SUBST([SNIPPET2])
AM_SUBST_NOTMAKE([SNIPPET2])

AC_CHECK_HEADERS([stdlib.h string.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(sqrt)

dnl Gimp-plugin
if test x$with_gimp = xyes ; then
	PKG_CHECK_MODULES(GIMP, gimp-2.0 gimpui-2.0)

	AC_SUBST(GIMP_CFLAGS)
	AC_SUBST(GIMP_LIBS)

	GIMP_LIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0`
	AC_SUBST(GIMP_LIBDIR)

	DATADIR='${datadir}/${PLUGIN_NAME}'

	AC_SUBST(DATADIR)

	AC_MSG_CHECKING([if GTK+ is version 2.3.0 or newer])
	if $PKG_CONFIG --atleast-version=2.3.0 gtk+-2.0; then
		have_gtk_2_3=yes
	else
		have_gtk_2_3=no
	fi
	AC_MSG_RESULT($have_gtk_2_3)

	if test "x$have_gtk_2_3" != "xyes"; then
		CPPFLAGS="$CPPFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
	fi
fi

#--------------------------------------------------------------------------
# Pass variables to MAKEFILE.AM
AC_SUBST([PLUGIN_MAJOR_VERSION],[plugin_major_version])
AC_SUBST([PLUGIN_MINOR_VERSION],[plugin_minor_version])
AC_SUBST([PLUGIN_VERSION],[plugin_version])
AC_SUBST([PLUGIN_PACKAGE_NAME],[plugin_package_email])
AC_SUBST([PLUGIN_EMAIL],[plugin_package_email])
AC_SUBST([HOST],["$host"])

# Put ifndef wrapper on refocus-it-config.h so we don't call it repeatedly.
AH_TOP([#ifndef REFOCUS_IT_CONFIG_H
#define REFOCUS_IT_CONFIG_H 1])
AH_BOTTOM([

#endif])

#--------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
po/Makefile
src/Makefile
gimp-plugin/Makefile
])

AC_OUTPUT
