if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libwriterperfect-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@.la

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_includedir = \
	$(includedir)/libwriterperfect-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@/libwriterperfect

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_include_HEADERS = \
	OdfDocumentHandler.hxx \
	OdtGenerator.hxx \
	OdgGenerator.hxx

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_CXXFLAGS = $(WRITERPERFECTCORE_CFLAGS) $(DEBUG_CXXFLAGS)
libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_LIBADD = @LIBWRITERPERFECT_WIN32_RESOURCE@ $(WRITERPERFECTCORE_LIBS)
libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_DEPENDENCIES = @LIBWRITERPERFECT_WIN32_RESOURCE@
libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_SOURCES = \
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

@LIBWRITERPERFECT_WIN32_RESOURCE@ : libwriterperfect.rc $(libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libwriterperfect.rc @LIBWRITERPERFECT_WIN32_RESOURCE@

endif

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libwriterperfect.rc
