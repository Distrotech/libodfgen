bin_PROGRAMS = wpd2odt wpg2odg

AM_CXXFLAGS = $(WRITERPERFECT_CXXFLAGS) $(DEBUG_CXXFLAGS) -I../filter -I..

wpd2odt_DEPENDENCIES = @WPD2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
wpd2odt_LDADD = $(WRITERPERFECT_LIBS) @WPD2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
if STATIC_TOOLS
wpd2odt_LDFLAGS = -all-static
endif

wpd2odt_SOURCES =			\
	DiskDocumentHandler.hxx		\
	DiskDocumentHandler.cxx		\
	GSFStream.cxx			\
	GSFStream.hxx			\
	StdOutHandler.hxx		\
	StdOutHandler.cxx		\
	wpd2odt.cxx

wpg2odg_DEPENDENCIES = @WPG2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
wpg2odg_LDADD = $(WRITERPERFECT_LIBS) @WPG2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
if STATIC_TOOLS
wpg2odg_LDFLAGS = -all-static
endif

wpg2odg_SOURCES =			\
	DiskDocumentHandler.hxx		\
	DiskDocumentHandler.cxx		\
	GSFStream.cxx			\
	GSFStream.hxx			\
	StdOutHandler.hxx		\
	StdOutHandler.cxx		\
	wpg2odg.cxx


if OS_WIN32

@WPD2ODT_WIN32_RESOURCE@ : wpd2odt.rc $(wpd2odt_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpd2odt.rc @WPD2ODT_WIN32_RESOURCE@

@WPG2ODG_WIN32_RESOURCE@ : wpg2odg.rc $(wpg2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpg2odg.rc @WPG2ODG_WIN32_RESOURCE@
endif 
