## Process this file with automake to produce Makefile.in

SUBDIRS = po src gimp-plugin

EXTRA_DIST = img/defocus.jpg img/defocus.pgm img/wilber.png \
	img/restored1.jpg img/restored2.jpg img/restored3.jpg \
	img/restored.pgm AUTHORS README README.md
nodist_EXTRA_DATA = .git .github .dep .lib
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules --enable-debugtime

DISTCLEANFILES =

strip:
	${STRIP} ${builddir}/gimp-plugin/refocus-it

