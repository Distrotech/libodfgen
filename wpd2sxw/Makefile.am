bin_PROGRAMS = wpd2sxw

AM_CXXFLAGS = $(WPD2SXW_CXXFLAGS) $(DEBUG_CXXFLAGS) -I../filter 

wpd2sxw_LDADD = $(WPD2SXW_LIBS) -L../filter -lwriterperfect-1

wpd2sxw_SOURCES =			\
	DiskDocumentHandler.hxx		\
	DiskDocumentHandler.cxx		\
	StdOutHandler.hxx		\
	StdOutHandler.cxx		\
	wpd2sxw.cxx
