; NSIS script to create a View3D binary installer for Windows
; by John Pye, 2009.
;
;--------------------------------

; The name of the installer

!ifndef VERSION
!define VERSION 0.svn
!endif

Name "View3D ${VERSION}"

;SetCompressor /FINAL zlib
SetCompressor /SOLID lzma

!include LogicLib.nsh
!include nsDialogs.nsh

; The file to write
!ifdef OUTFILE
OutFile ${OUTFILE}
!else
OutFile "view3d-${VERSION}.exe"
!endif


; The default installation directory
InstallDir $PROGRAMFILES\View3D

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\View3D" "Install_Dir"

;--------------------------------

; Pages

Page license
LicenseData "..\LICENSE.txt"

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; .onInit has been moved to after section decls so that they can be references

;------------------------------------------------------------------------
; INSTALL CORE STUFF including model library

; The stuff to install
Section "View3D (required)"
	SectionIn RO

	DetailPrint "--- COMMON FILES ---"

	; Set output path to the installation directory.
	SetOutPath $INSTDIR
	File "..\view3d.dll"
	File "..\view3d.exe"
	File "..\view2d.exe"
	File "..\viewer.exe"
	File "..\viewer2d.exe"
	File "..\viewht.exe"
	File "..\LICENSE.txt"
	File "..\CHANGELOG.txt"
	File "..\README.txt"
	File "..\VIEW3D-USER-MANUAL.pdf"
	
	SetOutPath $INSTDIR\examples\3d
	File "..\examples\box345.vs3"
	File "..\examples\gunter1.vs3"
	File "..\examples\gunter2.vs3"
	File "..\examples\wdwdoor.vs3"
	File "..\examples\facet.vs3"
	File "..\examples\test4.vs3"
	File "..\examples\test4a.vs3"
	
	SetOutPath $INSTDIR\examples\2d
	File ..\test\2d\test.vs2
	File ..\test\2d\test1.vs2  
	File ..\test\2d\test2.vs2  
	File ..\test\2d\test3.vs2  
	File ..\test\2d\test4.vs2  
	File ..\test\2d\test5.vs2
	File ..\test\2d\test6.vs2  
	File ..\test\2d\test7.vs2  
	File ..\test\2d\test8.vs2
	File ..\test\2d\test9.vs2
	File ..\test\2d\test10.vs2  
	File ..\test\2d\test11.vs2
	File ..\test\2d\facet.vs2

	SetOutPath $INSTDIR\examples\viewht
	File ..\test\viewht\README.txt
	File ..\test\viewht\output-expected.txt
	File ..\test\viewht\test-temperatures.txt
	File ..\test\viewht\test-viewfactors.txt
		
	; Write the installation path into the registry
	WriteRegStr HKLM SOFTWARE\View3D "Install_Dir" "$INSTDIR"

	; Write the uninstall keys for Windows
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\View3D" "DisplayName" "View3D (calculates radiation view factors)"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\View3D" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\View3D" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\View3D" "NoRepair" 1
	WriteUninstaller "uninstall.exe"
	
	Return
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts" sect_menu
  
	WriteRegDWORD HKLM "SOFTWARE\View3D" "StartMenu" 1

	CreateDirectory "$SMPROGRAMS\View3D"  
	
	; Documentation
	CreateShortCut "$SMPROGRAMS\View3D\User's Manual.lnk" "$INSTDIR\VIEW3D-USER-MANUAL.pdf" "" "$INSTDIR\VIEW3D-USER-MANUAL.pdf" 0

	; Information files
	CreateShortCut "$SMPROGRAMS\View3D\LICENSE.lnk" "$INSTDIR\LICENSE.txt" '' "$INSTDIR\LICENSE.txt" 0
	CreateShortCut "$SMPROGRAMS\View3D\CHANGELOG.lnk" "$INSTDIR\CHANGELOG.txt" '' "$INSTDIR\CHANGELOG.txt" 0
	CreateShortCut "$SMPROGRAMS\View3D\README.lnk" "$INSTDIR\README.txt" '' "$INSTDIR\README.txt" 0

	CreateShortCut "$SMPROGRAMS\View3D\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;------------------------------------------------------------------
; UNINSTALLER

Section "Uninstall"
	
;--- start menu ---

	ReadRegDWORD $0 HKLM "SOFTWARE\View3D" "StartMenu"
	${If} $0 <> 0
		; Remove shortcuts, if any
		DetailPrint "--- REMOVING START MENU SHORTCUTS ---"
		RmDir /r "$SMPROGRAMS\View3D"
	${EndIf}

;--- common components ---

	DetailPrint "--- REMOVING COMMON COMPONENTS ---"
	; Remove registry keys

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\View3D"
	DeleteRegKey HKLM "SOFTWARE\View3D"

	; Remove files and uninstaller

	Delete $INSTDIR\view3d.dll
	Delete $INSTDIR\view3d.exe
	Delete $INSTDIR\view2d.exe
	Delete $INSTDIR\viewer.exe
	Delete $INSTDIR\viewer2d.exe
	Delete $INSTDIR\viewht.exe
	Delete $INSTDIR\LICENSE.txt
	Delete $INSTDIR\README.txt
	Delete $INSTDIR\CHANGELOG.txt
	Delete $INSTDIR\VIEW3D-USER-MANUAL.pdf
	
	Delete $INSTDIR\examples\3d\box345.vs3
	Delete $INSTDIR\examples\3d\gunter1.vs3
	Delete $INSTDIR\examples\3d\gunter2.vs3
	Delete $INSTDIR\examples\3d\wdwdoor.vs3
	Delete $INSTDIR\examples\3d\facet.vs3
	Delete $INSTDIR\examples\3d\test4.vs3
	Delete $INSTDIR\examples\3d\test4a.vs3
	RMDir $INSTDIR\examples\3d
	Delete $INSTDIR\examples\2d\test.vs2
	Delete $INSTDIR\examples\2d\test1.vs2  
	Delete $INSTDIR\examples\2d\test2.vs2  
	Delete $INSTDIR\examples\2d\test3.vs2  
	Delete $INSTDIR\examples\2d\test4.vs2  
	Delete $INSTDIR\examples\2d\test5.vs2
	Delete $INSTDIR\examples\2d\test6.vs2  
	Delete $INSTDIR\examples\2d\test7.vs2  
	Delete $INSTDIR\examples\2d\test8.vs2
	Delete $INSTDIR\examples\2d\test9.vs2
	Delete $INSTDIR\examples\2d\test10.vs2  
	Delete $INSTDIR\examples\2d\test11.vs2  
	Delete $INSTDIR\examples\2d\facet.vs2  
	RMDir $INSTDIR\examples\2d
	Delete $INSTDIR\examples\viewht\README.txt
	Delete $INSTDIR\examples\viewht\output-expected.txt
	Delete $INSTDIR\examples\viewht\test-temperatures.txt
	Delete $INSTDIR\examples\viewht\test-viewfactors.txt
	RMDir $INSTDIR\examples\viewht
	RMDir $INSTDIR\examples

	Delete $INSTDIR\uninstall.exe

	; Remove directory
	RMDir $INSTDIR

SectionEnd

Function .onInit
	; Nothing here at the moment
FunctionEnd
