#if PLATFORM_WIN32
#no_undefined = -no-undefined
#else
#no_undefined =
#endif

version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

#if OS_WIN32
#install-libtool-import-lib:
#	$(INSTALL) .libs/libwpd-1.dll.a $(DESTDIR)$(libdir)
#	$(INSTALL) .libs/libwpd-stream-1.dll.a $(DESTDIR)$(libdir)

#uninstall-libtool-import-lib:
#	-rm $(DESTDIR)$(libdir)/libwpd-1.dll.a
#	-rm $(DESTDIR)$(libdir)/libwpd-stream-1.dll.a

#else
#install-libtool-import-lib:
#uninstall-libtool-import-lib:
#endif

lib_LTLIBRARIES = libwriterperfect-1.la 
libwriterperfect_1_includedir = $(includedir)/writerperfect-1/writerperfect
libwriterperfect_1_include_HEADERS = WordPerfectCollector.hxx

AM_CXXFLAGS = $(WRITERPERFECT_CXXFLAGS) $(DEBUG_CXXFLAGS)

#libwpd_1_la_LIBADD  = @LIBWPD_WIN32_RESOURCE@
#libwpd_1_la_DEPENDENCIES = @LIBWPD_WIN32_RESOURCE@
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

#if OS_WIN32

#@LIBWPD_WIN32_RESOURCE@ : libwpd.rc
#	$(top_srcdir)/build/win32/lt-compile-resource libwpd.rc @LIBWPD_WIN32_RESOURCE@

#@LIBWPD_STREAM_WIN32_RESOURCE@ : libwpd-stream.rc
#	$(top_srcdir)/build/win32/lt-compile-resource libwpd-stream.rc @LIBWPD_STREAM_WIN32_RESOURCE@
#endif


install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib

# Include the GSFStream.* in case we build a tarball without stream
#EXTRA_DIST = \
#	$(libwpd_stream_sources)	\
#	$(libwpd_stream_headers)	\
#	makefile.mk			\
#	libwpd.rc.in			\
#	libwpd-stream.rc.in

# These may be in the builddir too
#BUILD_EXTRA_DIST = 		\
#	libwpd.rc		\
#	libwpd-stream.rc
