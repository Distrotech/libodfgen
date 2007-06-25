bin_PROGRAMS = wpd2odt

AM_CXXFLAGS = $(WPD2SXW_CXXFLAGS) $(DEBUG_CXXFLAGS) -I../filter 

wpd2odt_DEPENDENCIES = @WPD2SXW_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
wpd2odt_LDADD = $(WPD2SXW_LIBS) @WPD2SXW_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la

wpd2odt_SOURCES =			\
	DiskDocumentHandler.hxx		\
	DiskDocumentHandler.cxx		\
	StdOutHandler.hxx		\
	StdOutHandler.cxx		\
	wpd2odt.cxx

if OS_WIN32

@WPD2SXW_WIN32_RESOURCE@ : wpd2odt.rc
	$(top_srcdir)/build/win32/lt-compile-resource wpd2odt.rc @WPD2SXW_WIN32_RESOURCE@
endif 
