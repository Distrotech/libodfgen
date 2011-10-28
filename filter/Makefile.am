if USE_LIBWPG
WP_WPG_SOURCES = \
	OdgGenerator.cxx \
	OdgGenerator.hxx
WP_WPG_CFLAGS = \
	$(WRITERPERFECTWPG_CFLAGS)
WP_WPG_LIBS = \
	$(WRITERPERFECTWPG_LIBS)
endif

noinst_LTLIBRARIES = libwriterperfect.la
libwriterperfect_la_CXXFLAGS = $(WRITERPERFECTCORE_CFLAGS) $(WP_WPG_CFLAGS) $(DEBUG_CXXFLAGS)
libwriterperfect_la_LIBADD  = $(WRITERPERFECTCORE_LIBS) $(WP_WPG_LIBS)
libwriterperfect_la_SOURCES = \
	FilterInternal.hxx \
	FilterInternal.cxx \
	Style.hxx \
	DocumentElement.cxx \
	DocumentElement.hxx \
	OdfDocumentHandler.hxx \
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
	OdtGenerator.hxx \
	$(WP_WPG_SOURCES)
