LANGUAGES = cs pt
PO_FILES = cs.po pt.po

# Information to help translators
# To create a new pot file, you need to cd into this po directory first:
#   cd po; del gimp30-refocus-it.pot; make update-pot
#
# To update existing po files you need to cd into po directory and then do:
#   cd po; make update-po

.PHONY: all install uninstall clean $(LANGUAGES)
.PHONY: update-po update-pot

all: $(LANGUAGES)

$(LANGUAGES):
	if [[ -e $(srcdir)/$@.po ]]; \
	then $(MSGFMT) -c -v -o $(builddir)/$@.mo $(srcdir)/$@.po; \
	else $(MSGFMT) -c -v -o $(builddir)/$@.mo $(builddir)/$@.po; \
	fi

update-po: $(PO_FILES)

$(PO_FILES): $(GETTEXT_PACKAGE).pot
	if [[ -e $(srcdir)/$@ ]]; \
	then $(MSGMERGE) -U $(srcdir)/$@ $^; \
	else $(MSGFMT) -l $(subst .po,,$@) -o $(builddir)/$@ -i $^; \
	fi

install-data-local: $(LANGUAGES)
	for L in $(LANGUAGES); \
	do $(MKDIR_P) -p "$(DESTDIR)$(LOCALEDIR)/$$L/LC_MESSAGES"; \
	install -v -m 0644 $(builddir)/$$L.mo "$(DESTDIR)$(LOCALEDIR)/$$L/LC_MESSAGES/$(GETTEXT_PACKAGE).mo"; \
	done

uninstall-local: $(LANGUAGES)
	for L in $(LANGUAGES); \
	do rm -vf "$(DESTDIR)$(LOCALEDIR)/$$L/LC_MESSAGES/$(GETTEXT_PACKAGE).mo"; \
	done

clean-local:
	rm -f *.po~ *.mo *.mo~

update-pot: $(GETTEXT_PACKAGE).pot

$(GETTEXT_PACKAGE).pot:
	$(XGETTEXT) -k_ -k_\" -kd_\( -c -d $(GETTEXT_PACKAGE) -o $@ \
	--package-version=$(PLUGIN_VERSION) --msgid-bugs-address=$(PLUGIN_EMAIL) --package-name=$(PLUGIN_PACKAGE_NAME) \
	$(top_srcdir)/gimp-plugin/*.c

EXTRA_DIST = $(PO_FILES) $(GETTEXT_PACKAGE).pot
