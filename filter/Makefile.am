if PLATFORM_WIN32
no_undefined = -no-undefined
else
no_undefined =
endif

version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

if OS_WIN32
install-libtool-import-lib:
	$(INSTALL) .libs/libwriterperfect-1.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libwriterperfect-1.dll.a

else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

lib_LTLIBRARIES = libwriterperfect-1.la 
libwriterperfect_1_includedir = $(includedir)/writerperfect-1/writerperfect
libwriterperfect_1_include_HEADERS = WordPerfectCollector.hxx DocumentElement.hxx

AM_CXXFLAGS = $(WRITERPERFECT_CXXFLAGS) $(DEBUG_CXXFLAGS)

libwriterperfect_1_la_LIBADD  = $(WRITERPERFECT_LIBS) @LIBWRITERPERFECT_WIN32_RESOURCE@
libwriterperfect_1_la_DEPENDENCIES = @LIBWRITERPERFECT_WIN32_RESOURCE@
libwriterperfect_1_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined) -
libwriterperfect_1_la_SOURCES =				\
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

if OS_WIN32

@LIBWRITERPERFECT_WIN32_RESOURCE@ : libwriterperfect.rc
	$(top_srcdir)/build/win32/lt-compile-resource libwriterperfect.rc @LIBWRITERPERFECT_WIN32_RESOURCE@

endif


install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib

EXTRA_DIST = \
	libwriterperfect.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = 		\
	libwriterperfect.rc
