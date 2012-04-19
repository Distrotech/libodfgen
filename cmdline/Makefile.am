if USE_LIBWPG
WPG_APP = wpg2odg
WPG_CFLAGS = $(WRITERPERFECTWPG_CFLAGS) -DUSE_LIBWPG=1
WPG_LIBS = $(WRITERPERFECTWPG_LIBS)
endif

if USE_LIBVISIO
VISIO_APP = vsd2odg
VISIO_CFLAGS = $(WRITERPERFECTVSD_CFLAGS)
VISIO_LIBS = $(WRITERPERFECTVSD_LIBS)
endif

if USE_LIBWPS
WPS_APP = wps2odt
WPS_CFLAGS = $(WRITERPERFECTWPS_CFLAGS)
WPS_LIBS = $(WRITERPERFECTWPS_LIBS)
endif

if USE_LIBCDR
CDR_APP = cdr2odg cmx2odg
CDR_CFLAGS = $(WRITERPERFECTCDR_CFLAGS)
CDR_LIBS = $(WRITERPERFECTCDR_LIBS)
endif

if USE_LIBMSPUB
PUB_APP = pub2odg
PUB_CFLAGS = $(WRITERPERFECTPUB_CFLAGS)
PUB_LIBS = $(WRITERPERFECTPUB_LIBS)
endif

if USE_GSF_OUTPUT
GSF_CFLAGS = $(WRITERPERFECTGSF_CFLAGS)
GSF_LIBS = $(WRITERPERFECTGSF_LIBS)
endif

bin_PROGRAMS = wpd2odt $(WPG_APP) $(VISIO_APP) $(WPS_APP) $(CDR_APP) $(PUB_APP)

noinst_LTLIBRARIES = libhelper.la
libhelper_la_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(WP_WPG_CFLAGS) $(DEBUG_CXXFLAGS)
libhelper_la_LIBADD  = $(WRITERPERFECTCORE_LIBS) $(WP_WPG_LIBS)
libhelper_la_SOURCES = \
	DiskDocumentHandler.hxx \
	DiskDocumentHandler.cxx \
	OutputFileHelper.cxx \
	OutputFileHelper.hxx \
	StdOutHandler.hxx \
	StdOutHandler.cxx \
	FemtoZip.cxx \
	FemtoZip.hxx

wpd2odt_DEPENDENCIES = @WPD2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
wpd2odt_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(WPG_CFLAGS) $(GSF_CFLAGS)
wpd2odt_LDADD = @WPD2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(WPG_LIBS) $(GSF_LIBS)
wpd2odt_SOURCES = \
	wpd2odt.cxx
if STATIC_TOOLS
wpd2odt_LDFLAGS = -all-static
endif

wps2odt_DEPENDENCIES = @WPS2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
wps2odt_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(WPS_CFLAGS) $(GSF_CFLAGS)
wps2odt_LDADD = @WPS2ODT_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(WPS_LIBS) $(GSF_LIBS)
wps2odt_SOURCES = \
	wps2odt.cxx
if STATIC_TOOLS
wps2odt_LDFLAGS = -all-static
endif

wpg2odg_DEPENDENCIES = @WPG2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
wpg2odg_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(WPG_CFLAGS) $(GSF_CFLAGS)
wpg2odg_LDADD = @WPG2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(WPG_LIBS) $(GSF_LIBS)
wpg2odg_SOURCES = \
	wpg2odg.cxx
if STATIC_TOOLS
wpg2odg_LDFLAGS = -all-static
endif


vsd2odg_DEPENDENCIES = @VSD2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
vsd2odg_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(VISIO_CFLAGS) $(GSF_CFLAGS)
vsd2odg_LDADD = @VSD2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(VISIO_LIBS) $(GSF_LIBS)
if STATIC_TOOLS
vsd2odg_LDFLAGS = -all-static
endif
vsd2odg_SOURCES = \
	vsd2odg.cxx

cdr2odg_DEPENDENCIES = @CDR2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
cdr2odg_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(CDR_CFLAGS) $(GSF_CFLAGS)
cdr2odg_LDADD = @CDR2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(CDR_LIBS) $(GSF_LIBS)
if STATIC_TOOLS
cdr2odg_LDFLAGS = -all-static
endif
cdr2odg_SOURCES = \
	cdr2odg.cxx

cmx2odg_DEPENDENCIES = @CMX2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
cmx2odg_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(CDR_CFLAGS) $(GSF_CFLAGS)
cmx2odg_LDADD = @CDR2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(CDR_LIBS) $(GSF_LIBS)
if STATIC_TOOLS
cmx2odg_LDFLAGS = -all-static
endif
cmx2odg_SOURCES = \
	cmx2odg.cxx

pub2odg_DEPENDENCIES = @PUB2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la
pub2odg_CXXFLAGS = -I$(top_srcdir)/filter -I../filter/ $(WRITERPERFECTCORE_CFLAGS) $(PUB_CFLAGS) $(GSF_CFLAGS)
pub2odg_LDADD = @PUB2ODG_WIN32_RESOURCE@ ../filter/libwriterperfect.la libhelper.la $(WRITERPERFECTCORE_LIBS) $(PUB_LIBS) $(GSF_LIBS)
if STATIC_TOOLS
pub2odg_LDFLAGS = -all-static
endif
pub2odg_SOURCES = \
	pub2odg.cxx

if OS_WIN32

@WPD2ODT_WIN32_RESOURCE@ : wpd2odt.rc $(wpd2odt_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpd2odt.rc @WPD2ODT_WIN32_RESOURCE@

@WPG2ODG_WIN32_RESOURCE@ : wpg2odg.rc $(wpg2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wpg2odg.rc @WPG2ODG_WIN32_RESOURCE@

@VSD2ODG_WIN32_RESOURCE@ : vsd2odg.rc $(vsd2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource vsd2odg.rc @VSD2ODG_WIN32_RESOURCE@

@WPS2ODT_WIN32_RESOURCE@ : wps2odt.rc $(wps2odt_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wps2odt.rc @WPS2ODT_WIN32_RESOURCE@

@CDR2ODG_WIN32_RESOURCE@ : cdr2odg.rc $(cdr2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cdr2odg.rc @CDR2ODG_WIN32_RESOURCE@

@CMX2ODG_WIN32_RESOURCE@ : cmx2odg.rc $(cmx2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource cmx2odg.rc @CMX2ODG_WIN32_RESOURCE@

@PUB2ODG_WIN32_RESOURCE@ : pub2odg.rc $(pub2odg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource pub2odg.rc @PUB2ODG_WIN32_RESOURCE@
endif 
