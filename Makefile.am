## Process this file with automake to produce Makefile.in

if REFOCUS_CMDLINE
TARGET_CMD      = cmd-line
else
TARGET_CMD      =
endif

if REFOCUS_GIMP
TARGET_GIMP     = gimp-plugin help
else
TARGET_GIMP     =
endif

SUBDIRS = po src doc $(TARGET_GIMP) $(TARGET_CMD)

EXTRA_DIST = BUGS autogen.sh

