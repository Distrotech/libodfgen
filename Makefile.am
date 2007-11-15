SUBDIRS = build filter cmdline

EXTRA_DIST = \
	writerperfect.spec.in \
	writerperfect.spec \
	writerperfect-zip.in \
	autogen.sh \
	config.h.in \
	CHANGES README

distclean-local:
	rm -rf *.cache *~ *.out

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

zip: install
	@sh writerperfect-zip
