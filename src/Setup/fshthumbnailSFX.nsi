!include "LogicLib.nsh"

; The name of the installer
Name "Fshthumbnailhandler SFX"



; The default installation directory
InstallDir $TEMP\FshThumbSetup

SilentInstall silent


Icon .\Setup_Install.ico

VIAddVersionKey ProductName "Fsh Thumbnail handler Setup"
VIAddVersionKey ProductVersion "1.2.5.0"
VIAddVersionKey FileVersion "1.2.5.0"
VIAddVersionKey LegalCopyright "Copyright � 2013 Nicholas Hayes"
VIAddVersionKey FileDescription "Fsh Thumbnail handler Setup"
VIProductVersion "1.2.5.0"

; The file to write

OutFile ".\nsisSetup\FshThumbnailhandlerSetup.exe"

;--------------------------------

; Pages

;Page directory
;Page instfiles

Var "Args"
!define DOT_MAJOR  "2"
!define DOT_MINOR  "0"
!define DOT_MINOR_MINOR  "50727"

;--------------------------------
; Usage
; Define in your script two constants:
;   DOT_MAJOR "(Major framework version)"
;   DOT_MINOR "{Minor framework version)"
;   DOT_MINOR_MINOR "{Minor framework version - last number after the second dot)"
; 
; Call IsDotNetInstalledAdv
; This function will abort the installation if the required version 
; or higher version of the .NETFramework is not installed.  Place it in
; either your .onInit function or your first install section before 
; other code.

Function IsDotNetInstalledAdv
   Push $0
   Push $1
   Push $2
   Push $3
   Push $4
   Push $5
 
  StrCpy $0 "0"
  StrCpy $1 "SOFTWARE\Microsoft\.NETFramework" ;registry entry to look in.
  StrCpy $2 0
 
  StartEnum:
    ;Enumerate the versions installed.
    EnumRegKey $3 HKLM "$1\policy" $2
 
    ;If we don't find any versions installed, it's not here.
    StrCmp $3 "" noDotNet notEmpty
 
    ;We found something.
    notEmpty:
      ;Find out if the RegKey starts with 'v'.  
      ;If it doesn't, goto the next key.
      StrCpy $4 $3 1 0
      StrCmp $4 "v" +1 goNext
      StrCpy $4 $3 1 1
 
      ;It starts with 'v'.  Now check to see how the installed major version
      ;relates to our required major version.
      ;If it's equal check the minor version, if it's greater, 
      ;we found a good RegKey.
      IntCmp $4 ${DOT_MAJOR} +1 goNext yesDotNetReg
      ;Check the minor version.  If it's equal or greater to our requested 
      ;version then we're good.
      StrCpy $4 $3 1 3
      IntCmp $4 ${DOT_MINOR} +1 goNext yesDotNetReg
 
      ;detect sub-version - e.g. 2.0.50727
      ;takes a value of the registry subkey - it contains the small version number
      EnumRegValue $5 HKLM "$1\policy\$3" 0
 
      IntCmpU $5 ${DOT_MINOR_MINOR} yesDotNetReg goNext yesDotNetReg
 
    goNext:
      ;Go to the next RegKey.
      IntOp $2 $2 + 1
      goto StartEnum
 
  yesDotNetReg: 
    ;Now that we've found a good RegKey, let's make sure it's actually
    ;installed by getting the install path and checking to see if the 
    ;mscorlib.dll exists.
    EnumRegValue $2 HKLM "$1\policy\$3" 0
    ;$2 should equal whatever comes after the major and minor versions 
    ;(ie, v1.1.4322)
    StrCmp $2 "" noDotNet
    ReadRegStr $4 HKLM $1 "InstallRoot"
    ;Hopefully the install root isn't empty.
    StrCmp $4 "" noDotNet
    ;build the actuall directory path to mscorlib.dll.
    StrCpy $4 "$4$3.$2\mscorlib.dll"
    IfFileExists $4 yesDotNet noDotNet
 
  noDotNet:
    ;Nope, something went wrong along the way.  Looks like the 
    ;proper .NETFramework isn't installed.  
 
    ;Uncomment the following line to make this function throw a message box right away 
     MessageBox MB_OK "You must have v${DOT_MAJOR}.${DOT_MINOR}.${DOT_MINOR_MINOR} or greater of the .NETFramework installed.  Aborting!"
     Abort
     StrCpy $0 0
     Goto done
 
     yesDotNet:
    ;Everything checks out.  Go on with the rest of the installation.
    StrCpy $0 1
 
   done:
     Pop $4
     Pop $3
     Pop $2
     Pop $1
     Exch $0
 FunctionEnd
 
 Function .onInit
 Call IsDotNetInstalledAdv
 FunctionEnd

; The stuff to install
Section "" 

; Set output path to the installation directory.
SetOutPath $INSTDIR

; Command-line parameters
Call GetParameters
Pop $Args

; Put file there
File "..\WinVista\Installer-x86\Release and package\FshThumbnailhandler.msi"
File "..\WinVista\Installer-x64\Release and package\FshThumbnailhandler-x64.msi"
File "..\thumbplug_fsh\FshthumbSetup-x64\Release\FshthumbSetup-x64.msi"
File "..\thumbplug_fsh\FshthumbSetup\Release\FshthumbSetup.msi"
File ".\launchmsi\bin\Release\launchmsi.exe"


ExecWait "launchmsi.exe"

Delete $INSTDIR\launchmsi.exe
Delete $INSTDIR\FshThumbnailhandler.msi
Delete $INSTDIR\FshThumbnailhandler-x64.msi
Delete $INSTDIR\FshthumbSetup.msi
Delete $INSTDIR\FshthumbSetup-x64.msi

RMDir /r $INSTDIR

SectionEnd

; GetParameters
; input, none
; output, top of stack (replaces, with e.g. whatever)
; modifies no other variables.
Function GetParameters

Push $R0
Push $R1
Push $R2
Push $R3

StrCpy $R2 1
StrLen $R3 $CMDLINE

;Check for quote or space
StrCpy $R0 $CMDLINE $R2
StrCmp $R0 '"' 0 +3
StrCpy $R1 '"'
Goto loop
StrCpy $R1 " "

loop:
IntOp $R2 $R2 + 1
StrCpy $R0 $CMDLINE 1 $R2
StrCmp $R0 $R1 get
StrCmp $R2 $R3 get
Goto loop

get:
IntOp $R2 $R2 + 1
StrCpy $R0 $CMDLINE 1 $R2
StrCmp $R0 " " get
StrCpy $R0 $CMDLINE "" $R2

Pop $R3
Pop $R2
Pop $R1
Exch $R0

FunctionEnd

