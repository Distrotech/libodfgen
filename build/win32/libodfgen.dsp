# Microsoft Developer Studio Project File - Name="libodfgen" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libodfgen - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libodfgen.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libodfgen.mak" CFG="libodfgen - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libodfgen - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libodfgen - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libodfgen - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\..\..\include\glib-2.0" /I "..\..\..\lib\glib-2.0\include" /I "..\..\..\include\libgsf-1" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\lib\libodfgen-1.lib"

!ELSEIF  "$(CFG)" == "libodfgen - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\..\..\include\glib-2.0" /I "..\..\..\lib\glib-2.0\include" /I "..\..\..\include\libgsf-1" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\lib\libodfgen-1.lib"

!ENDIF 

# Begin Target

# Name "libodfgen - Win32 Release"
# Name "libodfgen - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\filter\DocumentElement.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\FontStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\FilterInternal.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\ListStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\PageSpan.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\SectionStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\TableStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\TextRunStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\WordPerfectCollector.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\filter\DocumentElement.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\FilterInternal.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\FontStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\ListStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\PageSpan.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\SectionStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\Style.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\TableStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\TextRunStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\filter\WordPerfectCollector.hxx
# End Source File
# End Group
# End Target
# End Project
