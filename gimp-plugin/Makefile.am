## Process this file with automake to produce Makefile.in

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS =-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS	= ${CFLAGS} ${CPPFLAGS} ${GIMP_CFLAGS} ${GTK_CFLAGS}
AM_CPPFLAGS	= -I${top_builddir} -I${top_srcdir}/src -I${top_srcdir}/gimp_plugin

DEFS		= -DLOCALEDIR=\"$(LOCALEDIR)\" \
		  -DDATADIR=\""$(DATADIR)"\" \
		  @DEFS@

## This is the GIMP plug-in
bin_PROGRAMS		= refocus-it
bindir			= $(GIMP_BINDIR)
refocus_it_SOURCES	= main-gimp.c
refocus_it_LDADD	= $(top_builddir)/src/librefocus-it.a ${GIMP_LIBS} -lm
