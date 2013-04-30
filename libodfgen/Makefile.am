if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libodfgen-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@.la

libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_includedir = \
	$(includedir)/libodfgen-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@/libodfgen

libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_include_HEADERS = \
	OdfDocumentHandler.hxx \
	OdtGenerator.hxx \
	OdgGenerator.hxx

libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_CXXFLAGS = $(WPD_CFLAGS) $(WPG_CFLAGS) $(DEBUG_CXXFLAGS)
libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_LIBADD = @LIBWRITERPERFECT_WIN32_RESOURCE@ $(WPD_LIBS)
libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_DEPENDENCIES = @LIBWRITERPERFECT_WIN32_RESOURCE@
libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined

libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_SOURCES = \
	FilterInternal.hxx \
	FilterInternal.cxx \
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

@LIBWRITERPERFECT_WIN32_RESOURCE@ : libodfgen.rc $(libodfgen_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libodfgen.rc @LIBWRITERPERFECT_WIN32_RESOURCE@

endif

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libodfgen.rc
