## Process this file with automake to produce Makefile.in

SUBDIRS = po src gimp-plugin

metainfodir = ${datarootdir}/metainfo
metainfo_DATA = ${srcdir}/org.gimp.extension.refocus-it.metainfo.xml

EXTRA_DIST = img/defocus.jpg img/defocus.pgm img/wilber.png \
	img/restored1.jpg img/restored2.jpg img/restored3.jpg \
	img/restored.pgm AUTHORS README README.md \
	org.gimp.extension.refocus-it.metainfo.xml
nodist_EXTRA_DATA = .git .github .dep .lib
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules --enable-debugtime

DISTCLEANFILES =

strip:
	${STRIP} ${builddir}/gimp-plugin/refocus-it

