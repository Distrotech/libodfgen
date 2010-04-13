if USE_LIBWPG
writerperfectwpg_program = wpg2odg
writerperfectwpg_cxxflags = $(WRITERPERFECTWPG_CXXFLAGS)
writerperfectwpg_ldadd = $(WRITERPERFECTWPG_LIBS)
else
writerperfectwpg_program =
writerperfectwpg_cxxflags =
writerperfectwpg_ldadd =
endif

bin_PROGRAMS = wpd2odt $(writerperfectwpg_program)

if USE_GSF_OUTPUT
writerperfectgsf_cxxflags = $(WRITERPERFECTGSF_CXXFLAGS)
writerperfectgsf_ldadd = $(WRITERPERFECTGSF_LIBS)
else
writerperfectgsf_cxxflags =
writerperfectgsf_ldadd =
endif

AM_CXXFLAGS = $(WRITERPERFECTCORE_CXXFLAGS) $(writerperfectgsf_cxxflags) $(writerperfectwpg_cxxflags) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/filter -I$(top_srcdir)

wpd2odt_DEPENDENCIES = @WPD2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
wpd2odt_LDADD = @WPD2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la $(WRITERPERFECTCORE_LIBS) $(writerperfectwpg_ldadd) $(writerperfectgsf_ldadd)
if STATIC_TOOLS
wpd2odt_LDFLAGS = -all-static
endif

wpd2odt_SOURCES = \
	DiskDocumentHandler.hxx \
	DiskDocumentHandler.cxx \
	OutputFileHelper.cxx \
	OutputFileHelper.hxx \
	StdOutHandler.hxx \
	StdOutHandler.cxx \
	FemtoZip.cxx \
	FemtoZip.hxx \
	wpd2odt.cxx

wpg2odg_DEPENDENCIES = @WPG2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la
wpg2odg_LDADD = @WPG2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect-1.la $(WRITERPERFECTCORE_LIBS) $(writerperfectwpg_ldadd) $(writerperfectgsf_ldadd)
if STATIC_TOOLS
wpg2odg_LDFLAGS = -all-static
endif

wpg2odg_SOURCES = \
	DiskDocumentHandler.hxx \
	DiskDocumentHandler.cxx \
	OutputFileHelper.cxx \
	OutputFileHelper.hxx \
	StdOutHandler.hxx \
	StdOutHandler.cxx \
	FemtoZip.cxx \
	FemtoZip.hxx \
	wpg2odg.cxx


if OS_WIN32

@WPD2ODT_WIN32_RESOURCE@ : wpd2odt.rc $(wpd2odt_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpd2odt.rc @WPD2ODT_WIN32_RESOURCE@

@WPG2ODG_WIN32_RESOURCE@ : wpg2odg.rc $(wpg2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpg2odg.rc @WPG2ODG_WIN32_RESOURCE@
endif 
