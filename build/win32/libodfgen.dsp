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
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
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
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
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

SOURCE=..\..\src\DocumentElement.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\FontStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\GraphicStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\InternalHandler.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\ListStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\OdgGenerator.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\OdpGenerator.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\OdsGenerator.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\OdtGenerator.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\PageSpan.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\SectionStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\SheetStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\TableStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\..\src\TextRunStyle.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\DocumentElement.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\FilterInternal.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\FontStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\GraphicStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\InternalHandler.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libodfgen\libodfgen.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\ListStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libodfgen\OdfDocumentHandler.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libodfgen\OdgGenerator.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libodfgen\OdpGenerator.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libodfgen\OdsGenerator.hxx
# End Source File
# Begin Source File

SOURCE=..\..\inc\libodfgen\OdtGenerator.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\PageSpan.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\SectionStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\Style.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\SheetStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\TableStyle.hxx
# End Source File
# Begin Source File

SOURCE=..\..\src\TextRunStyle.hxx
# End Source File
# End Group
# End Target
# End Project
