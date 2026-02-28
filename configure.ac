# -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

# Copyright (C) 2026 by Jose Da Silva

AC_PREREQ([2.69])
#--------------------------------------------------------------------------
# Setup these variables before running AC_INIT

m4_define([plugin_name], [refocus-it])
m4_define([plugin_major_version], [2])
m4_define([plugin_minor_version], [2])
m4_define([plugin_version], [plugin_major_version.plugin_minor_version])
m4_define([plugin_package_name], [gimp3-refocus-it])
m4_define([plugin_package_home], [https://github.com/JoesCat/gimp3-refocus-it])
m4_define([plugin_package_email], [https://github.com/JoesCat/gimp3-refocus-it/issues])
#--------------------------------------------------------------------------
AC_INIT([plugin_name], [plugin_version], [plugin_package_email],
	[plugin_package_name], [plugin_package_home])
#--------------------------------------------------------------------------
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
AC_CHECK_PROGS([GIMP],[gimp-3],[:])
AC_CHECK_PROGS([GIMPTOOL],[gimptool-3],[:])
AC_PATH_PROG([MD5SUM],[md5sum],[:])
AC_PATH_PROG([MSGFMT],[msgfmt],[:])
AC_PATH_PROG([MSGINIT],[msginit],[:])
AC_PATH_PROG([MSGMERGE],[msgmerge],[:])
AC_PATH_PROG([PKG_CONFIG],[pkg-config],[:])
AC_PATH_PROG([STRIP],[strip],[:])
AC_PATH_PROG([XGETTEXT],[xgettext],[:])
AM_CONFIG_HEADER(refocus-it-config.h)
AC_PROG_INSTALL
AC_PROG_MAKE_SET

#--------------------------------------------------------------------------
# Enable silent build rules by default, this requires atleast Automake-1.11
# Disable by passing --disable-silent-rules to configure or using make V=1
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],[AC_SUBST([AM_DEFAULT_VERBOSITY],[1])])

#--------------------------------------------------------------------------
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
AC_CHECK_HEADERS([stdlib.h string.h strings.h])
AC_SEARCH_LIBS([strerror],[cposix])

# Search for math.h include and math lib (some OSes have -lm built-in).
have_libm=maybe
AC_CHECK_HEADERS([math.h],
  AC_SEARCH_LIBS([sqrt],[m],[have_libm=yes],
    AC_CHECK_FUNC([sqrt],[have_libm=yes])))
if test x"${have_libm}" != xyes; then
  AC_MSG_FAILURE([ERROR: Please install the Math library and math.h],[1])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(sqrt)

#--------------------------------------------------------------------------
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
GETTEXT_PACKAGE=gimp30-refocus-it
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext translation domain.])
if test x"${have_gettext}" = xyes; then
  AC_DEFINE([HAVE_GETTEXT],1,[Enable use of local languages])
fi

#--------------------------------------------------------------------------
# Check for libgimp/gimp.h include file and libgimp library.
GIMP_CFLAGS=
GIMP_LIBS=
have_libgimp=no
PKG_CHECK_MODULES([GIMP],[gimp-3.0 >= 3.0.0 gimpui-3.0 >= 3.0.0],[have_libgimp=yes])
if test x"${have_libgimp}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgimp3.],[1])
fi
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)

# Pass GIMP_LIBDIR to automake for default GIMP plug-ins directory
# Seems gimp doesn't follow libdir of the distro, so just replaced
# prefix and sending it as GIMP_BINDIR
gimp3_gimplibdir=`${PKG_CONFIG} --variable=gimplibdir gimp-3.0`
gimp3_prefix=`${PKG_CONFIG} --variable=exec_prefix gimp-3.0`
gimp3_relative=${gimp3_gimplibdir#/usr}
dnl we want target directory as /usr not /usr/local
if test "$prefix" = "/usr/local" || test "$prefix" = "NONE" ;
then
  gimp3_bindir=\${exec_prefix}/.."$gimp3_relative"
else
  gimp3_bindir=\${exec_prefix}"$gimp3_relative"
fi
GIMP_BINDIR=${gimp3_bindir}/plug-ins/refocus-it
AC_SUBST(GIMP_BINDIR)

AM_CONDITIONAL([HAVEGIMPTOOL],[test "${GIMPTOOL}"x != x])

GTK_CFLAGS=
GTK_LIBS=
have_libgtk=no
PKG_CHECK_MODULES([GTK],[gtk+-3.0],[have_libgtk=yes])
if test x"${have_libgtk}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install the developer version of libgtk+3.],[1])
fi
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

#--------------------------------------------------------------------------
dnl DEBUG
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
  [Enable debugging @<:@default=no@:>@])],
  [],[enable_debug=no])
if test x"${enable_debug}" = xyes || test x"${enable_debug}" = xtrue ; then
  AC_DEFINE([NDEBUG],1,[Enable debugging messages printf to commandline])
fi

DATADIR='${datadir}/${PLUGIN_NAME}'
AC_SUBST(DATADIR)

CPPFLAGS="${CPPFLAGS} AS_ESCAPE([-I${top_builddir}]) AS_ESCAPE([-I${top_srcdir}])"
AC_SUBST([CPPFLAGS],["${CPPFLAGS}"])

#--------------------------------------------------------------------------
# Pass more variables to refocus-it-config.h
AC_DEFINE(PLUGIN_NAME,["plugin_name"],[Plug-In name])
AC_DEFINE(PLUGIN_EMAIL,[plugin_package_email],[Plug-In package email])
AC_DEFINE(PACKAGE_NAME,["plugin_package_name"],[Plug-In Package name])
AC_DEFINE(PLUGIN_VERSION,["plugin_version"],[Plug-In version])

AC_DEFINE([PLUGIN_MAJOR_VERSION], plugin_major_version,[Plug-In major version])
AC_DEFINE([PLUGIN_MINOR_VERSION], plugin_minor_version,[Plug-In minor version])

# Put ifndef wrapper on refocus-it-config.h so we don't call it repeatedly.
AH_TOP([#ifndef REFOCUS_IT_CONFIG_H
#define REFOCUS_IT_CONFIG_H 1])
AH_BOTTOM([

#endif])
#--------------------------------------------------------------------------
# Pass more variables to MAKEFILE.AM
AC_SUBST([PLUGIN_MAJOR_VERSION],[plugin_major_version])
AC_SUBST([PLUGIN_MINOR_VERSION],[plugin_minor_version])
AC_SUBST([PLUGIN_VERSION],[plugin_version])
AC_SUBST([PLUGIN_PACKAGE_NAME],[plugin_package_email])
AC_SUBST([PLUGIN_EMAIL],[plugin_package_email])
AC_SUBST([HOST],["$host"])

#--------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
po/Makefile
src/Makefile
gimp-plugin/Makefile
])

AC_OUTPUT
AC_MSG_NOTICE([

Configuration:

  Source code location	${srcdir}
  Build code location	${builddir}
  docs root dir		${datarootdir}
  GIMP3 locale dir	${LOCALEDIR}
  Use locale languages	${have_gettext}
  Compiler		${CC}
  CPPFLAGS		${CPPFLAGS}
  CFLAGS		${CFLAGS}
  LIBS			${LIBS}

  prefix		${prefix}
  exec_prefix		${exec_prefix}
  refocus-it bindir	${GIMP_BINDIR}
  GIMP_CFLAGS		${GIMP_CFLAGS}
  GIMP_LIBS		${GIMP_LIBS}
  GTK_CFLAGS		${GTK_CFLAGS}
  GTK_LIBS		${GTK_LIBS}
])
