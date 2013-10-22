ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig



SUBDIRS = build inc src

if WITH_LIBEBOOK_DOCS
SUBDIRS += docs
endif

pkgconf_DATA = libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.pc

EXTRA_DIST = \
	autogen.sh \
	config.h.in \
	libodfgen.pc.in \
	NEWS README COPYING.MPL COPYING.LGPL

distclean-local:
	rm -rf *.cache *~ *.out *.pc

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog
