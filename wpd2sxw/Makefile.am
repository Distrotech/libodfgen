bin_PROGRAMS = wpd2sxw

AM_CXXFLAGS = $(WPD2SXW_CXXFLAGS) $(DEBUG_CXXFLAGS) -I../filter 

wpd2sxw_DEPENDENCIES = @WPD2SXW_WIN32_RESOURCE@ 
wpd2sxw_LDADD = $(WPD2SXW_LIBS) -L../filter -lwriterperfect-1 @WPD2SXW_WIN32_RESOURCE@ 

wpd2sxw_SOURCES =			\
	DiskDocumentHandler.hxx		\
	DiskDocumentHandler.cxx		\
	StdOutHandler.hxx		\
	StdOutHandler.cxx		\
	wpd2sxw.cxx

if OS_WIN32

@WPD2SXW_WIN32_RESOURCE@ : wpd2sxw.rc
	$(top_srcdir)/build/win32/lt-compile-resource wpd2sxw.rc @WPD2SXW_WIN32_RESOURCE@
endif 
