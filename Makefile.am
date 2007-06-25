SUBDIRS = build filter ooo packaging wpd2odt

EXTRA_DIST =	\
	wpd2odt.spec.in			\
	wpd2odt.spec			\
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
