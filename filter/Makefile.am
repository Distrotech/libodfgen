if USE_LIBWPG

writerperfect_wpg_sources = \
	OdgGenerator.cxx			\
	OdgGenerator.hxx

writerperfect_wpg_cxxflags = \
	$(WRITERPERFECTWPG_CXXFLAGS)
	
writerperfect_wpg_ldflags = \
	$(WRITERPERFECTWPG_LIBS)

else

writerperfect_wpg_sources = 
writerperfect_wpg_cxxflags =
writerperfect_wpg_ldflags =

endif
	
noinst_LTLIBRARIES = libwriterperfect-1.la

AM_CXXFLAGS = $(WRITERPERFECTCORE_CXXFLAGS) $(writerperfect_wpg_cxxflags) $(DEBUG_CXXFLAGS)

libwriterperfect_1_la_LIBADD  = $(WRITERPERFECTCORE_LIBS) $(writerperfect_wpg_ldflags)
libwriterperfect_1_la_SOURCES =		\
	FilterInternal.hxx		\
	Style.hxx			\
	WriterProperties.hxx		\
	DocumentElement.cxx 		\
	DocumentElement.hxx 		\
	OdfDocumentHandler.hxx		\
	FontStyle.cxx			\
	FontStyle.hxx			\
	InternalHandler.cxx		\
	InternalHandler.hxx		\
	ListStyle.cxx			\
	ListStyle.hxx			\
	PageSpan.cxx			\
	PageSpan.hxx			\
	SectionStyle.cxx 		\
	SectionStyle.hxx		\
	TableStyle.cxx 			\
	TableStyle.hxx			\
	TextRunStyle.cxx 		\
	TextRunStyle.hxx		\
	OdtGenerator.cxx 	\
	OdtGenerator.hxx	\
	$(writerperfect_wpg_sources)	

