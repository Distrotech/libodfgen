SUBDIRS = build filter ooo packaging cmdline

EXTRA_DIST =	\
	writerperfect.spec.in		\
	writerperfect.spec		\
	autogen.sh			\
	Makefile.ooo-external		\
	Makefile.ooo-external.msvc.in	\
	CHANGES README \
	README.ooo-external.msvc \
	OOoSDK.win32.diff

distclean-local:
	rm -rf *.cache *~ *.out

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz
