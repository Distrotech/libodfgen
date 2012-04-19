# Process this file with autoconf to create configure.

AC_PREREQ([2.65])

# ====================
# Version informations
# ====================
m4_define([writerperfect_version_major],[0])
m4_define([writerperfect_version_minor],[8])
m4_define([writerperfect_version_micro],[1])
m4_define([writerperfect_version],[writerperfect_version_major.writerperfect_version_minor.writerperfect_version_micro])

# =============
# Automake init
# =============
AC_INIT([writerperfect],[writerperfect_version])
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([1.11 foreign dist-xz dist-bzip2])
AM_SILENT_RULES([yes])
AC_LANG([C++])

# ===========================
# Find required base packages
# ===========================
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_LIBTOOL_WIN32_DLL
PKG_PROG_PKG_CONFIG([0.20])

# ====================
# Find additional apps
# ====================
PKG_CHECK_MODULES([WRITERPERFECTCORE],[
    libwpd-0.9
    libwpd-stream-0.9
])

# =================================
# Libtool/Version Makefile settings
# =================================
AC_SUBST(WRITERPERFECT_MAJOR_VERSION, [writerperfect_version_major])
AC_SUBST(WRITERPERFECT_MINOR_VERSION, [writerperfect_version_minor])
AC_SUBST(WRITERPERFECT_MICRO_VERSION, [writerperfect_version_micro])
AC_SUBST(WRITERPERFECT_VERSION, [writerperfect_version])
# AC_SUBST(LT_RELEASE, [writerperfect_version_major.writerperfect_version_minor])
LT_CURRENT=`expr 100 '*' writerperfect_version_major + writerperfect_version_minor`
# For 1.0.0 comment the first line and uncomment the second
LT_AGE=0
# LT_AGE=writerperfect_version_minor
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION, [writerperfect_version_micro])
AC_SUBST(LT_AGE)

# ==========================
# Platform check for windows
# ==========================
AC_MSG_CHECKING([for native Win32])
AS_CASE([$host],
	[*-*-mingw*], [
		native_win32=yes
		AC_CHECK_TOOL(WINDRES, windres)
		WPD2ODT_WIN32_RESOURCE=wpd2odt-win32res.lo
		WPG2ODG_WIN32_RESOURCE=wpg2odg-win32res.lo
		VSD2ODG_WIN32_RESOURCE=vsd2odg-win32res.lo
		WPS2ODT_WIN32_RESOURCE=wps2odt-win32res.lo
		CDR2ODG_WIN32_RESOURCE=cdr2odg-win32res.lo
		CMX2ODG_WIN32_RESOURCE=cmx2odg-win32res.lo
		PUB2ODG_WIN32_RESOURCE=pub2odg-win32res.lo
		MWAW2ODT_WIN32_RESOURCE=mwaw2odt-win32res.lo
	], [
		native_win32=no
		WPD2ODT_WIN32_RESOURCE=
		WPG2ODG_WIN32_RESOURCE=
		VSD2ODG_WIN32_RESOURCE=
		WPS2ODT_WIN32_RESOURCE=
		CDR2ODG_WIN32_RESOURCE=
		CMX2ODG_WIN32_RESOURCE=
		PUB2ODG_WIN32_RESOURCE=
		MWAW2ODT_WIN32_RESOURCE=
	]
)
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL([OS_WIN32], [test "x$native_win32" = "xyes"])
AC_SUBST(WPD2ODT_WIN32_RESOURCE)
AC_SUBST(WPG2ODG_WIN32_RESOURCE)
AC_SUBST(VSD2ODG_WIN32_RESOURCE)
AC_SUBST(WPS2ODT_WIN32_RESOURCE)
AC_SUBST(CDR2ODG_WIN32_RESOURCE)
AC_SUBST(CMX2ODG_WIN32_RESOURCE)
AC_SUBST(PUB2ODG_WIN32_RESOURCE)
AC_SUBST(MWAW2ODT_WIN32_RESOURCE)

AC_MSG_CHECKING([for Win32 platform in general])
AS_CASE([$host],
	[*-*-mingw*|*-*-cygwin*], [platform_win32=yes],
	[platform_win32=no]
)
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL([PLATFORM_WIN32], [test "x$platform_win32" = "xyes"])

# ================
# Check for cflags
# ================
AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--disable-werror], [Treat all warnings as errors, usefull for development])],
	[enable_werror="$enableval"],
	[enable_werror=yes]
)
AS_IF([test x"$enable_werror" != "xno"], [
	CFLAGS="$CFLAGS -Werror"
	CXXFLAGS="$CXXFLAGS -Werror"
])

# ============
# Debug switch
# ============
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug], [Turn on debugging])],
	[enable_debug="$enableval"],
	[enable_debug=no]
)
AS_IF([test "x$enable_debug" = "xyes"], [
	DEBUG_CXXFLAGS="-DDEBUG -g"
], [
	DEBUG_CXXFLAGS="-DNDEBUG"
])
AC_SUBST(DEBUG_CXXFLAGS)

# ============
# Static tools
# ============
AC_ARG_ENABLE([static-tools],
	[AS_HELP_STRING([--enable-static-tools], [Link tools (binaries) statically])],
	[enable_static_tools="$enableval"],
	[enable_static_tools=no]
)
AS_IF([test "x$enable_static_tools" = "xyes"], [
	enable_static="yes"
], [
	AC_DISABLE_STATIC
])
AM_CONDITIONAL(STATIC_TOOLS, [test "x$enable_static_tools" = "xyes"])

# ======
# libgsf
# ======
AC_ARG_WITH(libgsf,
	[AS_HELP_STRING([--without-libgsf],
		[Use libgsf for output of structured files])],
	[with_libgsf="$withval"],
	[with_libgsf=auto]
)
AS_IF([test "x$with_libgsf" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTGSF, [
			libgsf-1 >= 1.12.0
			glib-2.0 >= 2.0.0
		], [have_libgsf=yes], [have_libgsf=no])
	], [have_libgsf=no])
AM_CONDITIONAL([USE_GSF_OUTPUT], [test "x$have_libgsf" = "xyes"])
AM_CONDITIONAL([GSF_HAS_COMPRESSION_LEVEL], [test "x$have_libgsf" = "xyes"])
AS_IF([test "x$have_libgsf" = "xno" -a "x$with_libgsf" = "xyes"], [
	AC_MSG_ERROR([libgsf requested but not found])
])

# ======
# libwpg
# ======
AC_ARG_WITH(libwpg,
	[AS_HELP_STRING([--without-libwpg],
		[Do not use libwpg for conversion of WordPerfect Graphics (embedded images will not be converted)])],
	[with_libwpg="$withval"],
	[with_libwpg=yes]
)
AS_IF([test "x$with_libwpg" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTWPG, [libwpg-0.2],
			[have_libwpg=yes], [have_libwpg=no])
	], [have_libwpg=no])
AM_CONDITIONAL([USE_LIBWPG], [test "x$have_libwpg" = "xyes"])
AS_IF([test "x$have_libwpg" = "xno" -a "x$with_libwpg" = "xyes"], [
	AC_MSG_ERROR([libwpg requested but not found])
])

# ======
# libwps
# ======
AC_ARG_WITH(libwps,
	[AS_HELP_STRING([--without-libwps],
		[Do not use for conversion of Microsoft Works Documents])],
	[with_libwps="$withval"],
	[with_libwps=auto]
)
AS_IF([test "x$with_libwps" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTWPS, [libwps-0.2],
			[have_libwps=yes], [have_libwps=no])
	], [have_libwps=no])
AM_CONDITIONAL([USE_LIBWPS], [test "x$have_libwps" = "xyes"])
AS_IF([test "x$have_libwps" = "xno" -a "x$with_libwps" = "xyes"], [
	AC_MSG_ERROR([libwps requested but not found])
])

# ========
# libvisio
# ========
AC_ARG_WITH(libvisio,
	[AS_HELP_STRING([--without-libvisio],
		[Use libvisio for conversion of Microsoft Visio Documents])],
	[with_libvisio="$withval"],
	[with_libvisio=auto]
)
AS_IF([test "x$with_libvisio" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTVSD, [libvisio-0.0],
			[have_libvisio=yes], [have_libvisio=no])
	], [have_libvisio=no])
AM_CONDITIONAL([USE_LIBVISIO], [test "x$have_libvisio" = "xyes"])
AS_IF([test "x$have_libvisio" = "xno" -a "x$with_libvisio" = "xyes"], [
	AC_MSG_ERROR([libvisio requested but not found])
])

# ========
# libcdr
# ========
AC_ARG_WITH(libcdr,
	[AS_HELP_STRING([--without-libcdr],
		[Use libcdr for conversion of Corel Draw Documents])],
	[with_libcdr="$withval"],
	[with_libcdr=auto]
)
AS_IF([test "x$with_libcdr" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTCDR, [libcdr-0.0 >= 0.0.5],
			[have_libcdr=yes], [have_libcdr=no])
	], [have_libcdr=no])
AM_CONDITIONAL([USE_LIBCDR], [test "x$have_libcdr" = "xyes"])
AS_IF([test "x$have_libcdr" = "xno" -a "x$with_libcdr" = "xyes"], [
	AC_MSG_ERROR([libcdr requested but not found])
])

# ========
# libmspub
# ========
AC_ARG_WITH(libmspub,
	[AS_HELP_STRING([--without-libmspub],
		[Use libmspub for conversion of Microsoft Publisher Documents])],
	[with_libmspub="$withval"],
	[with_libmspub=auto]
)
AS_IF([test "x$with_libmspub" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTPUB, [libmspub-0.0],
			[have_libmspub=yes], [have_libmspub=no])
	], [have_libmspub=no])
AM_CONDITIONAL([USE_LIBMSPUB], [test "x$have_libmspub" = "xyes"])
AS_IF([test "x$have_libmspub" = "xno" -a "x$with_libmspub" = "xyes"], [
	AC_MSG_ERROR([libmspub requested but not found])
])

# ======
# libmwaw
# ======
AC_ARG_WITH(libmwaw,
	[AS_HELP_STRING([--without-libmwaw],
		[Do not use for conversion of Mac Classic Text Documents (MacWrite(tm), WriteNow(tm), ...)])],
	[with_libmwaw="$withval"],
	[with_libmwaw=auto]
)
AS_IF([test "x$with_libmwaw" != "xno"], [
		PKG_CHECK_MODULES(WRITERPERFECTMWAW, [libmwaw-0.0],
			[have_libmwaw=yes], [have_libmwaw=no])
	], [have_libmwaw=no])
AM_CONDITIONAL([USE_LIBMWAW], [test "x$have_libmwaw" = "xyes"])
AS_IF([test "x$have_libmwaw" = "xno" -a "x$with_libmwaw" = "xyes"], [
	AC_MSG_ERROR([libmwaw requested but not found])
])

# ========
# shared_ptr implementation
# ========
AC_ARG_WITH(sharedptr,
	[AS_HELP_STRING([--with-sharedptr=boost|c++11|tr1],
		[Use Boost|std::memory|std::tr1::memory for shared ptr implementation])],
	[with_sharedptr="$withval"],
	[with_sharedptr=boost]
)
AS_IF([test "x$with_sharedptr" == "xc++11"], [
		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM(
			[[#include <memory>]],
			[[std::shared_ptr<int> p;]]
      	)],
		[AC_DEFINE([SHAREDPTR_STD],[1],[Use C++11 implementation of shared_ptr])],
		[AC_MSG_ERROR(Could not find C++11 implementation of shared_ptr)],
		[[#include <memory>]])
	]
)
AS_IF([test "x$with_sharedptr" == "xtr1"], [
		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM(
			[[#include <tr1/memory>]],
			[[std::tr1::shared_ptr<int> p;]]
      	)],
		[AC_DEFINE([SHAREDPTR_TR1],[1],[Use TR1 implementation of shared_ptr])],
		[AC_MSG_ERROR(Could not find TR1 implementation of shared_ptr)],
		[[#include <tr1/memory>]])
	]
)
AS_IF([test "x$with_sharedptr" == "xboost"], [
		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM(
			[[#include <boost/shared_ptr.hpp>]],
			[[boost::shared_ptr<int> p;]]
      	)],
		[AC_DEFINE([SHAREDPTR_BOOST],[1],[Use Boost implementation of shared_ptr],
		[AC_MSG_ERROR(Could not find Boost implementation of shared_ptr)])])
	]
)


# =====================
# Prepare all .in files
# =====================
AC_CONFIG_FILES([
Makefile
writerperfect.spec
writerperfect-zip
build/Makefile
build/win32/Makefile
filter/Makefile
cmdline/Makefile
cmdline/cdr2odg.rc
cmdline/cmx2odg.rc
cmdline/mwaw2odt.rc
cmdline/pub2odg.rc
cmdline/vsd2odg.rc
cmdline/wpd2odt.rc
cmdline/wpg2odg.rc
cmdline/wps2odt.rc
])
AC_OUTPUT

# ==============================================
# Display final informations about configuration
# ==============================================
AC_MSG_NOTICE([
==============================================================================
Build configuration:
	debug:           ${enable_debug}
	libcdr:          ${have_libcdr}
	libgsf:          ${have_libgsf}
	libmspub:        ${have_libmspub}
	libmwaw:         ${have_libmwaw}
	libvisio:        ${have_libvisio}
	libwpg:          ${have_libwpg}
	libwps:          ${have_libwps}
	werror:          ${enable_werror}
==============================================================================
])
