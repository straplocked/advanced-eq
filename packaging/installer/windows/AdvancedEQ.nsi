; Advanced EQ NSIS Installer Script
; Version is passed via /DVERSION=x.y.z at compile time

!include "MUI2.nsh"

Name "Advanced EQ ${VERSION}"
OutFile "AdvancedEQ-${VERSION}-Windows.exe"
InstallDir "$PROGRAMFILES\Advanced EQ"
RequestExecutionLevel admin

; UI settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\..\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; --- Install Sections ---

Section "VST3 Plugin" SecVST3
    SectionIn RO ; Required
    SetOutPath "$COMMONFILES\VST3"
    File /r "${BUILD_DIR}\AdvancedEQ_artefacts\Release\VST3\Advanced EQ.vst3"
SectionEnd

Section "Standalone Application" SecStandalone
    SetOutPath "$INSTDIR"
    File "${BUILD_DIR}\AdvancedEQ_artefacts\Release\Standalone\Advanced EQ.exe"

    ; Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\Advanced EQ"
    CreateShortcut "$SMPROGRAMS\Advanced EQ\Advanced EQ.lnk" "$INSTDIR\Advanced EQ.exe"
    CreateShortcut "$SMPROGRAMS\Advanced EQ\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "-Uninstaller"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AdvancedEQ" "DisplayName" "Advanced EQ ${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AdvancedEQ" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AdvancedEQ" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AdvancedEQ" "Publisher" "Advanced EQ"
SectionEnd

; --- Component descriptions ---

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecVST3} "VST3 plugin for use in any VST3-compatible DAW."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecStandalone} "Standalone application that runs without a DAW."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; --- Uninstaller ---

Section "Uninstall"
    RMDir /r "$COMMONFILES\VST3\Advanced EQ.vst3"
    RMDir /r "$INSTDIR"
    RMDir /r "$SMPROGRAMS\Advanced EQ"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AdvancedEQ"
SectionEnd
