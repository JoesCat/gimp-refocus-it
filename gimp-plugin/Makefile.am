## Process this file with automake to produce Makefile.in

BUILDDIR	= $(top_builddir)/src
SRCDIR		= $(top_srcdir)/src

# The braces around ACLOCAL_FLAGS below instead of parentheses are intentional!
# Otherwise autoreconf misparses the line.
ACLOCAL_AMFLAGS =-I m4 ${ACLOCAL_FLAGS}
AM_CFLAGS	= ${CFLAGS} ${CPPFLAGS} ${GIMP_CFLAGS} ${GTK_CFLAGS}
AM_CPPFLAGS	= -I$(top_srcdir)/gimp_plugin -I$(top_srcdir) -I$(SRCDIR)

DEFS		= -DLOCALEDIR=\"$(LOCALEDIR)\" \
		  -DDATADIR=\""$(DATADIR)"\" \
		  @DEFS@

## This is the GIMP plug-in
bin_PROGRAMS		= refocus-it
bindir			= $(GIMP_BINDIR)/plug-ins/refocus-it
refocus_it_SOURCES	= main-gimp.c
refocus_it_LDADD	= $(BUILDDIR)/librefocus-it.a ${GIMP_LIBS} -lm
