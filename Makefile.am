ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig

SUBDIRS = build filter cmdline lo

pkgconf_DATA = libwriterperfect-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@.pc

EXTRA_DIST = \
	writerperfect.spec.in \
	writerperfect.spec \
	writerperfect-zip.in \
	autogen.sh \
	config.h.in \
	NEWS README

distclean-local:
	rm -rf *.cache *~ *.out

rpm: dist
	rpmbuild --nodeps -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.*

zip: install
	@sh writerperfect-zip

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog
