if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la

libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_includedir = \
	$(includedir)/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@/libodfgen

libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_include_HEADERS = \
	$(top_srcdir)/inc/libodfgen/OdfDocumentHandler.hxx \
	$(top_srcdir)/inc/libodfgen/OdtGenerator.hxx \
	$(top_srcdir)/inc/libodfgen/OdgGenerator.hxx \
	$(top_srcdir)/inc/libodfgen/libodfgen.hxx

AM_CXXFLAGS = -I$(top_srcdir)/inc/ $(WPD_CFLAGS) $(WPG_CFLAGS) $(DEBUG_CXXFLAGS)

libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_la_LIBADD = @LIBODFGEN_WIN32_RESOURCE@ $(WPD_LIBS)
libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_la_DEPENDENCIES = @LIBODFGEN_WIN32_RESOURCE@
libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined

libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_la_SOURCES = \
	FilterInternal.hxx \
	Style.hxx \
	DocumentElement.cxx \
	DocumentElement.hxx \
	FontStyle.cxx \
	FontStyle.hxx \
	InternalHandler.cxx \
	InternalHandler.hxx \
	ListStyle.cxx \
	ListStyle.hxx \
	PageSpan.cxx \
	PageSpan.hxx \
	SectionStyle.cxx \
	SectionStyle.hxx \
	TableStyle.cxx \
	TableStyle.hxx \
	TextRunStyle.cxx \
	TextRunStyle.hxx \
	OdtGenerator.cxx \
	OdgGenerator.cxx

if OS_WIN32

@LIBODFGEN_WIN32_RESOURCE@ : libodfgen.rc $(libodfgen_@LIBODFGEN_MAJOR_VERSION@_@LIBODFGEN_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libodfgen.rc @LIBODFGEN_WIN32_RESOURCE@

endif

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libodfgen.rc
