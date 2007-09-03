bin_PROGRAMS = wpd2sxw

AM_CXXFLAGS = $(WPD2SXW_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/filter 

wpd2sxw_DEPENDENCIES = @WPD2SXW_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
wpd2sxw_LDADD = $(WPD2SXW_LIBS) @WPD2SXW_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
if STATIC_TOOLS
wpd2sxw_LDFLAGS = -all-static
endif

wpd2sxw_SOURCES =			\
	DiskDocumentHandler.hxx		\
	DiskDocumentHandler.cxx		\
	StdOutHandler.hxx		\
	StdOutHandler.cxx		\
	wpd2sxw.cxx

if OS_WIN32

@WPD2SXW_WIN32_RESOURCE@ : wpd2sxw.rc $(wpd2sxw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpd2sxw.rc @WPD2SXW_WIN32_RESOURCE@
endif 
