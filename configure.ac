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
	], [
		native_win32=no
		WPD2ODT_WIN32_RESOURCE=
		WPG2ODG_WIN32_RESOURCE=
		VSD2ODG_WIN32_RESOURCE=
		WPS2ODT_WIN32_RESOURCE=
	]
)
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL([OS_WIN32], [test "x$native_win32" = "xyes"])
AC_SUBST(WPD2ODT_WIN32_RESOURCE)
AC_SUBST(WPG2ODG_WIN32_RESOURCE)
AC_SUBST(VSD2ODG_WIN32_RESOURCE)
AC_SUBST(WPS2ODT_WIN32_RESOURCE)

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
	[AS_HELP_STRING([--with-libgsf],
		[Use libgsf for output of structured files])],
	[with_libgsf="$withval"],
	[with_libgsf=no]
)
AM_CONDITIONAL([USE_GSF_OUTPUT], [test "x$with_libgsf" = "xyes"])
AM_CONDITIONAL([GSF_HAS_COMPRESSION_LEVEL], [test "x$with_libgsf" = "xyes"])
AS_IF([test "x$with_libgsf" = "xyes"], [
	PKG_CHECK_MODULES([WRITERPERFECTGSF], [
		libgsf-1 >= 1.12.0
		glib-2.0 >= 2.0.0
	])
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
AM_CONDITIONAL([USE_LIBWPG], [test "x$with_libwpg" != "xno"])
AS_IF([test "x$with_libwpg" != "xno"], [
	PKG_CHECK_MODULES(WRITERPERFECTWPG, [
		libwpg-0.2
	])
])

# ======
# libwps
# ======
AC_ARG_WITH(libwps,
	[AS_HELP_STRING([--without-libwps],
		[Do not use for conversion of Microsoft Works Documents])],
	[with_libwps="$withval"],
	[with_libwps=yes]
)
AM_CONDITIONAL([USE_LIBWPS], [test "x$with_libwps" != "xno"])
AS_IF([test "x$with_libwps" != "xno"], [
	PKG_CHECK_MODULES(WRITERPERFECTWPS, [
		libwps-0.2
	])
])

# ========
# libvisio
# ========
AC_ARG_WITH(libvisio,
	[AS_HELP_STRING([--with-libvisio],
		[Use libvisio for conversion of Microsoft Visio Documents])],
	[with_libvisio="$withval"],
	[with_libvisio=no]
)
AM_CONDITIONAL([USE_LIBVISIO], [test "x$with_libvisio" = "xyes"])
AS_IF([test "x$with_libvisio" = "xyes"], [
	PKG_CHECK_MODULES(WRITERPERFECTVSD, [
		libvisio-0.0
	])
])

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
cmdline/wpd2odt.rc
cmdline/wpg2odg.rc
cmdline/vsd2odg.rc
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
	libgsf:          ${with_libgsf}
	libwpg:          ${with_libwpg}
	libwps:          ${with_libwps}
	libvisio:        ${with_libvisio}
	werror:          ${enable_werror}
==============================================================================
])
