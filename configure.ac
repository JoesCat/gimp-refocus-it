dnl Process this file with autoconf to produce a configure script.

# Copyright (C) 2025 by Jose Da Silva

AC_PREREQ(2.69)

dnl Name your plug-in here
m4_define([plugin_name], [refocus-it])

dnl These two define the plug-in version number
m4_define([plugin_major_version], [2])
m4_define([plugin_minor_version], [1])

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
  AC_SEARCH_LIBS([cos],[m],[have_libm=yes],
    AC_CHECK_FUNC([cos],[have_libm=yes])))
if test x"${have_libm}" != xyes; then
  AC_MSG_FAILURE([ERROR: Please install the Math library and math.h],[1])
fi

dnl DEBUG
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
  [Enable debugging @<:@default=no@:>])],
  [],[enable_debug=no])

if test x"${enable_debug}" = xyes || test x"${enable_debug}" = xtrue ; then
  AC_DEFINE([NDEBUG],1,[Enable debugging messages printf to commandline])
fi

dnl i18n stuff
LOCALEDIR='${datadir}/locale'
AC_SUBST(LOCALEDIR)

# Avoid being locked to a particular gettext verion, use what's available.
have_gettext=no
AC_CHECK_HEADERS([intl.h],[have_gettext=yes])
AC_CHECK_FUNC([gettext],[have_gettext=yes],[have_gettext=no])
if test x"${have_gettext}" = xno; then
  AC_SEARCH_LIBS([intl],[have_gettext=yes],[
  AC_MSG_ERROR([ERROR: gettext() required! Please install libintl or GETTEXT Packages.])])
fi
AC_CHECK_HEADERS([locale.h],,[have_gettext=no])
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

AC_CHECK_HEADERS([stdlib.h string.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(sqrt)

dnl Gimp-plugin
# Check for libgimp/gimp.h include file and libgimp library.
GIMP_CFLAGS=
GIMP_LIBS=
have_libgimp=no
PKG_CHECK_MODULES([GIMP],[gimp-2.0 >= 2.10.0 gimpui-2.0 >= 2.10.0],[have_libgimp=yes])
if test x"${have_libgimp}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgimp2.],[1])
fi
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)

# Pass GIMP_LIBDIR to automake for default GIMP plug-ins directory
GIMP_GIMPLIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0`
# Seems that gimp does not follow libdir of the distro, so just replacing prefix
GIMP_LIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0`
AC_SUBST(GIMP_LIBDIR)
GIMP_PREFIX=`$PKG_CONFIG --variable=exec_prefix gimp-2.0`
GIMP_RELATIVE=${GIMP_GIMPLIBDIR#$GIMP_PREFIX}
GIMP_BINDIR=\${exec_prefix}"$GIMP_RELATIVE"
AC_SUBST(GIMP_BINDIR)

DATADIR='${datadir}/${PLUGIN_NAME}'
AC_SUBST(DATADIR)

GTK_CFLAGS=
GTK_LIBS=
have_libgtk=no
PKG_CHECK_MODULES([GTK],[gtk+-2.0],[have_libgtk=yes])
if test x"${have_libgtk}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgtk+2.],[1])
fi
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

CPPFLAGS="${CPPFLAGS} AS_ESCAPE([-I${top_builddir}]) AS_ESCAPE([-I${top_srcdir}])"
AC_SUBST([CPPFLAGS],["${CPPFLAGS}"])

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
