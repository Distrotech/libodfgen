if PLATFORM_WIN32
no_undefined = -no-undefined
else
no_undefined =
endif

version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

install-libtool-import-lib:
uninstall-libtool-import-lib:

lib_LTLIBRARIES = libwriterperfect-1.la 
libwriterperfect_1_includedir = $(includedir)/writerperfect-1/writerperfect
libwriterperfect_1_include_HEADERS = WordPerfectCollector.hxx

AM_CXXFLAGS = $(WRITERPERFECT_CXXFLAGS) $(DEBUG_CXXFLAGS)

libwriterperfect_1_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined)
libwriterperfect_1_la_SOURCES =				\
	DocumentElement.hxx		\
	FilterInternal.hxx		\
	FontStyle.hxx			\
	ListStyle.hxx			\
	PageSpan.hxx			\
	SectionStyle.hxx		\
	Style.hxx			\
	TableStyle.hxx			\
	TextRunStyle.hxx		\
	WordPerfectCollector.hxx	\
	WriterProperties.hxx		\
	DocumentElement.cxx 		\
	FontStyle.cxx			\
	ListStyle.cxx			\
	PageSpan.cxx			\
	SectionStyle.cxx 		\
	TableStyle.cxx 			\
	TextRunStyle.cxx 		\
	WordPerfectCollector.cxx 	

install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib

