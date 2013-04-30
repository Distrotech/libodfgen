# Process this file with autoconf to create configure.

AC_PREREQ([2.65])

# ====================
# Version informations
# ====================
m4_define([libodfgen_version_major],[0])
m4_define([libodfgen_version_minor],[0])
m4_define([libodfgen_version_micro],[0])
m4_define([libodfgen_version],[libodfgen_version_major.libodfgen_version_minor.libodfgen_version_micro])

# =============
# Automake init
# =============
AC_INIT([libodfgen],[libodfgen_version])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
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
LT_INIT([win32-dll disable-static pic-only])
AC_CANONICAL_HOST
PKG_PROG_PKG_CONFIG([0.20])

# ====================
# Find additional apps
# ====================
PKG_CHECK_MODULES([WPD],[
	libwpd-0.9
	libwpd-stream-0.9
])
AC_SUBST([WPD_CFLAGS])
AC_SUBST([WPD_LIGS])

PKG_CHECK_MODULES([WPG],[
	libwpg-0.2
])
AC_SUBST([WPG_CFLAGS])
AC_SUBST([WPG_LIGS])

# =================================
# Libtool/Version Makefile settings
# =================================
AC_SUBST(LIBODFGEN_MAJOR_VERSION, [libodfgen_version_major])
AC_SUBST(LIBODFGEN_MINOR_VERSION, [libodfgen_version_minor])
AC_SUBST(LIBODFGEN_MICRO_VERSION, [libodfgen_version_micro])
# AC_SUBST(LT_RELEASE, [libodfgen_version_major.libodfgen_version_minor])
LT_CURRENT=`expr 100 '*' libodfgen_version_major + libodfgen_version_minor`
# For 1.0.0 comment the first line and uncomment the second
LT_AGE=0
# LT_AGE=libodfgen_version_minor
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION, [libodfgen_version_micro])
AC_SUBST(LT_AGE)

# ==========================
# Platform check for windows
# ==========================
AC_MSG_CHECKING([for native Win32])
AS_CASE([$host],
	[*-*-mingw*], [
		native_win32=yes
		AC_CHECK_TOOL(WINDRES, windres)
		LIBODFGEN_WIN32_RESOURCE=libodfgen-win32res.lo
	], [
		native_win32=no
		LIBODFGEN_WIN32_RESOURCE=
	]
)
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL([OS_WIN32], [test "x$native_win32" = "xyes"])
AC_SUBST(LIBODFGEN_WIN32_RESOURCE)

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
# Courtesy of Glib: Ensure MSVC-compatible struct packing convention
# is used when compiling for Win32 with gcc.
AS_IF([test "x$native_win32" = "xyes"], [
	AC_CHECK_TOOL(WINDRES, windres)
	AS_IF([test x"$GCC" = xyes], [
		AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
		AS_IF([test -z "$ac_cv_prog_CC"], [
			our_gcc="$CC"
		], [
			our_gcc="$ac_cv_prog_CC"
		])
		AS_IF([$our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null], [
			msnative_struct="-mms-bitfields"
			CFLAGS="$CFLAGS $msnative_struct"
			CXXFLAGS="$CXXFLAGS $msnative_struct"
			AC_MSG_RESULT([${msnative_struct}])
		], [
			AC_MSG_RESULT([no way])
			AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
		])
	])
	CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
	CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wshadow -pedantic"
], [
	AS_IF([test x"$GCC" = xyes], [
		# Be tough with warnings and produce less careless code
		CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
		CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wshadow -pedantic"
	])
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

# ========
# shared_ptr implementation
# ========
AC_ARG_WITH(sharedptr,
	[AS_HELP_STRING([--with-sharedptr=boost|c++11|tr1],
		[Use Boost|std::memory|std::tr1::memory for shared ptr implementation])],
	[with_sharedptr="$withval"],
	[with_sharedptr=boost]
)
AS_CASE([$with_sharedptr],
	[boost|c++11|tr1], [],
	[AC_MSG_ERROR([Unknown value of shared_ptr provider: $with_sharedptr])]
)

AS_IF([test "x$with_sharedptr" = "xc++11"], [
	AC_MSG_CHECKING([for c++11 shared ptr])
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM(
			[[#include <memory>]],
			[[std::shared_ptr<int> p;]]
		)
	], [
		AC_MSG_RESULT([yes])
		AC_DEFINE([SHAREDPTR_STD],[1],[Use C++11 implementation of shared_ptr])
	], [
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Could not find C++11 implementation of shared_ptr])
	], [
		[#include <memory>]
	])
])
AS_IF([test "x$with_sharedptr" = "xtr1"], [
	AC_MSG_CHECKING([for tr1 shared ptr])
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM(
			[[#include <tr1/memory>]],
			[[std::tr1::shared_ptr<int> p;]]
		)
	], [
		AC_MSG_RESULT([yes])
		AC_DEFINE([SHAREDPTR_TR1],[1],[Use TR1 implementation of shared_ptr])
	], [
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Could not find TR1 implementation of shared_ptr])
	], [
		[#include <tr1/memory>]
	])
])
AS_IF([test "x$with_sharedptr" = "xboost"], [
	AC_MSG_CHECKING([for boost shared ptr])
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM(
			[[#include <boost/shared_ptr.hpp>]],
			[[boost::shared_ptr<int> p;]]
		)
	], [
		AC_MSG_RESULT([yes])
		AC_DEFINE([SHAREDPTR_BOOST],[1],[Use Boost implementation of shared_ptr])
	], [
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Could not find Boost implementation of shared_ptr])
	])
])


# =====================
# Prepare all .in files
# =====================
AC_CONFIG_FILES([
libodfgen-0.0.pc
Makefile
libodfgen.spec
libodfgen-zip
build/Makefile
build/win32/Makefile
src/Makefile
src/libodfgen.rc
])
AC_OUTPUT

# ==============================================
# Display final informations about configuration
# ==============================================
AC_MSG_NOTICE([
==============================================================================
Build configuration:
	debug:           ${enable_debug}
	shared_ptr:      ${with_sharedptr}
	werror:          ${enable_werror}
==============================================================================
])
