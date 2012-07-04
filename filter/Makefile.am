
lib_LTLIBRARIES = libwriterperfect-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@.la

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_includedir = \
	$(includedir)/libwriterperfect-@WRITERPERFECT_MAJOR_VERSION@.@WRITERPERFECT_MINOR_VERSION@/libwriterperfect

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_include_HEADERS = \
	OdfDocumentHandler.hxx \
	OdtGenerator.hxx \
	OdgGenerator.hxx

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_CXXFLAGS = $(WRITERPERFECTCORE_CFLAGS) $(DEBUG_CXXFLAGS)

libwriterperfect_@WRITERPERFECT_MAJOR_VERSION@_@WRITERPERFECT_MINOR_VERSION@_la_LIBADD  = $(WRITERPERFECTCORE_LIBS)

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
