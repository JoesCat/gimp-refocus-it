## Process this file with automake to produce Makefile.in

BUILDDIR	= $(top_builddir)/src
SRCDIR		= $(top_srcdir)/src

INCLUDES	= -I$(top_srcdir)/gimp_plugin -I$(top_srcdir) -I$(SRCDIR)

DEFS		= -DLOCALEDIR=\"$(LOCALEDIR)\" \
		  -DDATADIR=\""$(DATADIR)"\" \
		  @DEFS@

AM_CPPFLAGS	= @GIMP_CFLAGS@

## This is the GIMP plug-in
bin_PROGRAMS			= gimp_plugin-refocus-it
bindir				= $(GIMP_LIBDIR)/plug-ins
gimp_plugin_refocus_it_SOURCES	= main-gimp.c
gimp_plugin_refocus_it_LDADD	= $(BUILDDIR)/librefocus-it.a \
				  @GIMP_LIBS@ -lm
