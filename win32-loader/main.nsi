; Debian-Installer Loader
; Copyright (C) 2007,2008,2009  Robert Millan <rmh@aybabtu.com>
; Copyright (C) 2010            Didier Raboud <didier@raboud.com>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

; SetCompressor must _always_ be the first command
SetCompressor /SOLID lzma

!include l10n/templates/all.nsh

Name $(program_name)
Icon "swirl.ico"
UninstallIcon "swirl.ico"
XPStyle on
OutFile "win32-loader.exe"

; Needed to disable idiotic compatibility mode where Vista identifies itself
; as XP.  Otherwise useless (admin privilege is the default for installers).
RequestExecutionLevel admin

!include LogicLib.nsh
!include FileFunc.nsh
!include WinMessages.nsh
!include WinVer.nsh
!insertmacro GetRoot

!addplugindir plugins
!addplugindir plugins/cpuid
!addplugindir plugins/systeminfo

;--------------------------------

LicenseData $(license)

;--------------------------------

; Pages

Page license
Page custom ShowExpert
Page custom ShowRescue
Page custom ShowKernel
Page custom ShowGraphics
!ifdef NOCD
Page custom ShowBranch
!endif
Page custom ShowDesktop
Page custom ShowCustom
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

Var /GLOBAL c
Var /GLOBAL d
Var /GLOBAL preseed_cmdline
Var /GLOBAL preseed_cfg
Var /GLOBAL proxy
Var /GLOBAL arch
Var /GLOBAL bcdedit

Function .onInit
  InitPluginsDir

  ; needed by: timezones, keymaps, languages
  File /oname=$PLUGINSDIR\maps.ini maps.ini

  ; default to English
  StrCpy $LANGUAGE ${LANG_ENGLISH}

  ; Language selection dialog
  Push ""
!include l10n/templates/dialog.nsh
  Push unsupported
  Push "-- Not in this list --"

  Push A ; A means auto count languages
         ; for the auto count to work the first empty push (Push "") must remain
  LangDLL::LangDialog "Choose language" "Please choose the language used for \
the installation process. This language will be the default language for the \
final system."

  Pop $LANGUAGE
  ${If} $LANGUAGE == "cancel"
    Abort
  ${Endif}

  ; Note: Possible API abuse here.  Nsis *seems* to fallback sanely to English
  ; when $LANGUAGE == "unsupported", so we'll use that to decide wether to
  ; preseed later.

  Var /GLOBAL unsupported_language
  ${If} $LANGUAGE == "unsupported"
    StrCpy $unsupported_language true
    ; Translators: your language is supported by d-i, but not yet by nsis.
    ; Please get your translation in nsis before translating win32-loader.
    MessageBox MB_OK "Because your language is not supported by this \
stage of the installer, English will be used for now.  On the second \
(and last) stage of the install process, you will be offered a much \
wider choice, where your language is more likely to be present."
  ${Else}
    StrCpy $unsupported_language false
  ${Endif}
FunctionEnd

Function ShowExpert
; Do initialisations as early as possible, but not before license has been
; accepted unless absolutely necessary.

; ********************************************** Initialise $preseed_cmdline
  StrCpy $preseed_cmdline " "

!ifndef NOCD
; ********************************************** Initialise $d
  ${GetRoot} $EXEDIR $d

; ********************************************** Check win32-loader.ini presence
; before initialising $arch or $gtk
  IfFileExists $d\win32-loader.ini +3 0
    MessageBox MB_OK|MB_ICONSTOP "$(error_missing_ini)"
    Quit

; ********************************************** Display README.html if found
  ReadINIStr $0 $PLUGINSDIR\maps.ini "languages" "$LANGUAGE"
  IfFileExists $d\README.$0.html 0 +3
    StrCpy $0 README.$0.html
    Goto readme_file_found
  IfFileExists $d\README.html 0 readme_file_not_found
    StrCpy $0 README.html
    Goto readme_file_found

readme_file_found:
  ShowWindow $HWNDPARENT ${SW_MINIMIZE}
  ExecShell "open" "file://$d/$0"

readme_file_not_found:
!endif

; ********************************************** Initialise $arch
  test64::get_arch
  StrCpy $arch $0
!ifndef NOCD
  ReadINIStr $0 $d\win32-loader.ini "installer" "arch"
  ${If} "$0:$arch" == "amd64:i386"
    MessageBox MB_OK|MB_ICONSTOP $(amd64_on_i386)
    Quit
  ${Endif}
  ${If} "$0:$arch" == "i386:amd64"
    MessageBox MB_YESNO|MB_ICONQUESTION $(i386_on_amd64) IDNO +2
    Quit
    StrCpy $arch "i386"
  ${Endif}
!endif

  ; Windows version is another abort condition
  Var /GLOBAL windows_boot_method
  ${If} ${IsNT}
    StrCpy $windows_boot_method ntldr
    Goto windows_version_ok
  ${Endif}
  ${If} ${AtMostWinME}
    StrCpy $windows_boot_method direct
    Goto windows_version_ok
  ${Endif}
  ${If} ${AtMostWin2003}
    StrCpy $windows_boot_method ntldr
    Goto windows_version_ok
  ${Endif}
  ${If} ${AtMostWinVista}
; In the default install, "system partition" is not mounted.  We need a way
; around this before Windows 7 can be enabled.
;  ${OrIf} ${IsWin7}
    StrCpy $windows_boot_method bootmgr
    Goto windows_version_ok
  ${Endif}
  MessageBox MB_OK|MB_ICONSTOP $(unsupported_version_of_windows)
  Quit
windows_version_ok:

; ********************************************** Initialise $c
; We set it to the "System partition" (see http://en.wikipedia.org/wiki/System_partition_and_boot_partition)

  ${If} $windows_boot_method == ntldr
  ${OrIf} $windows_boot_method == bootmgr
    systeminfo::find_system_partition
    Pop $c
    ${If} $c == failed
      ${GetRoot} $WINDIR $c
      MessageBox MB_OK|MB_ICONEXCLAMATION $(cant_find_system_partition)
    ${Endif}
    Goto c_is_initialized
  ${Endif}
  ${If} $windows_boot_method == direct
    ; Doesn't really matter.
    ${GetRoot} $WINDIR $c
    Goto c_is_initialized
  ${Endif}
c_is_initialized:
  ; For the uninstaller
  WriteRegStr HKLM "Software\Debian\Debian-Installer Loader" "system_drive" "$c"

  StrCpy $INSTDIR "$c\win32-loader"
  SetOutPath $INSTDIR

  File /oname=$PLUGINSDIR\expert.ini	templates/binary_choice.ini
  WriteINIStr $PLUGINSDIR\expert.ini "Field 1" "Text" $(expert1)
  WriteINIStr $PLUGINSDIR\expert.ini "Field 2" "Text" $(expert2)
  WriteINIStr $PLUGINSDIR\expert.ini "Field 3" "Text" $(expert3)
  InstallOptions::dialog $PLUGINSDIR\expert.ini

  Var /GLOBAL expert
  ReadINIStr $0 $PLUGINSDIR\expert.ini "Field 3" "State"
  ${If} $0 == "1"
    StrCpy $expert true
  ${Else}
    StrCpy $expert false
  ${Endif}
FunctionEnd

Function ShowRescue
  File /oname=$PLUGINSDIR\rescue.ini	templates/binary_choice.ini
  WriteINIStr $PLUGINSDIR\rescue.ini "Field 1" "Text" $(rescue1)
  WriteINIStr $PLUGINSDIR\rescue.ini "Field 2" "Text" $(rescue2)
  WriteINIStr $PLUGINSDIR\rescue.ini "Field 3" "Text" $(rescue3)
  InstallOptions::dialog $PLUGINSDIR\rescue.ini

  ReadINIStr $0 $PLUGINSDIR\rescue.ini "Field 3" "State"
  ${If} $0 == "1"
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i rescue/enable boolean true"
  ${Endif}
FunctionEnd

Function ShowKernel
  Var /GLOBAL kernel 
!ifdef NOCD
  ${If} $expert == true
  ${AndIf} $windows_boot_method != direct ; loadlin can only load linux
    File /oname=$PLUGINSDIR\kernel.ini	templates/binary_choice.ini
    WriteINIStr $PLUGINSDIR\kernel.ini "Field 1" "Text" $(kernel1)
    WriteINIStr $PLUGINSDIR\kernel.ini "Field 2" "Text" $(kernel2)
    WriteINIStr $PLUGINSDIR\kernel.ini "Field 3" "Text" $(kernel3)
    InstallOptions::dialog $PLUGINSDIR\kernel.ini
 
    ReadINIStr $0 $PLUGINSDIR\kernel.ini "Field 3" "State"
    ${If} $0 == "1"
      StrCpy $kernel kfreebsd
    ${Else}
      StrCpy $kernel linux
    ${Endif}
  ${Else}
    ; ** Default to GNU/Linux
    StrCpy $kernel linux
  ${Endif}
!else
  ; TODO: detect the CD architecture
  StrCpy $kernel linux
!endif
FunctionEnd

Function ShowGraphics
  Var /GLOBAL user_interface
  Var /GLOBAL gtk

!ifndef NOCD
  Var /GLOBAL predefined_user_interface
  ReadINIStr $predefined_user_interface $d\win32-loader.ini "installer" "user_interface"

  ${If} $predefined_user_interface == ""
!endif
    ${If} $expert == true
     ${AndIf} $kernel == "linux"
      File /oname=$PLUGINSDIR\graphics.ini	templates/graphics.ini
      File /oname=$PLUGINSDIR\gtk.bmp	templates/gtk.bmp
      File /oname=$PLUGINSDIR\text.bmp	templates/text.bmp
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 1" "Text" $(graphics1)
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 2" "Text" "$PLUGINSDIR\gtk.bmp"
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 3" "Text" "$PLUGINSDIR\text.bmp"
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 4" "Text" $(graphics2)
      WriteINIStr $PLUGINSDIR\graphics.ini "Field 5" "Text" $(graphics3)
      InstallOptions::dialog $PLUGINSDIR\graphics.ini
      ReadINIStr $0 $PLUGINSDIR\graphics.ini "Field 4" "State"
      ${If} $0 == "1"
        StrCpy $user_interface graphical
      ${EndIf}
    ${ElseIf} $kernel == linux
      ; ** Default to graphical interface for linux
      StrCpy $user_interface graphical
    ${Endif}
!ifndef NOCD
  ${Else}
    StrCpy $user_interface $predefined_user_interface
  ${Endif}
!endif

${If} $user_interface == "graphical"
  StrCpy $gtk "gtk/"
${Endif}

; ********************************************** Preseed vga mode
  ${If} "$user_interface" == "graphical"
    StrCpy $preseed_cmdline "$preseed_cmdline video=vesa:ywrap,mtrr vga=788"
  ${Else}
    StrCpy $preseed_cmdline "$preseed_cmdline vga=normal"
  ${EndIf}
FunctionEnd

!ifdef NOCD

Function Download
    ; Base URL
    Pop $0
    ; Target dir
    Pop $1
    ; Filename
    Pop $2
    ; Allow it to fail?
    Pop $3
    Var /GLOBAL nsisdl_proxy
    ${If} $proxy == ""
      StrCpy $nsisdl_proxy /NOIEPROXY
    ${Else}
      StrCpy $nsisdl_proxy "/PROXY $proxy"
    ${Endif}

    NSISdl::download /TRANSLATE $(nsisdl1) $(nsisdl2) $(nsisdl3) $(nsisdl4) $(nsisdl5) $(nsisdl6) $(nsisdl7) $(nsisdl8) $nsisdl_proxy $0/$2 $1\$2
    Pop $R0
    ${If} $R0 != "success"
      ${If} $R0 != "cancel"
         MessageBox MB_OK|MB_ICONSTOP "$(error): $R0"
      ${EndIf}
      ${If} $3 == "false"
        Quit
      ${Endif}
    ${EndIf}
    StrCpy $0 "$R0"
FunctionEnd

Function ShowBranch
  Var /GLOBAL di_branch
  StrCpy $di_branch stable
  File /oname=$PLUGINSDIR\di_branch.ini	templates/binary_choice.ini
  ${If} $expert == true
    WriteINIStr $PLUGINSDIR\di_branch.ini "Field 1" "Text" $(di_branch1)
    WriteINIStr $PLUGINSDIR\di_branch.ini "Field 2" "Text" $(di_branch2)
    WriteINIStr $PLUGINSDIR\di_branch.ini "Field 3" "Text" $(di_branch3)
    InstallOptions::dialog $PLUGINSDIR\di_branch.ini
    ReadINIStr $0 $PLUGINSDIR\di_branch.ini "Field 3" "State"
    ${If} $0 == "1"
      StrCpy $di_branch daily
    ${Endif}
  ${Endif}

; ********************************************** Initialise base_url
  Var /GLOBAL base_url
  ReadINIStr $0 $PLUGINSDIR\di_branch.ini "Field 3" "State"
  ${If} $di_branch == "daily"
    MessageBox MB_YESNO|MB_ICONQUESTION $(di_branch4) IDNO +2
    ExecShell "open" "http://wiki.debian.org/DebianInstaller/Today"

    ; Daily images URL
    ; See http://svn.debian.org/viewsvn/d-i/trunk/scripts/daily-build-aggregator for the canonical list
    ${If} $kernel == "linux"
      ${If} $arch == "i386"
        StrCpy $base_url "http://people.debian.org/~joeyh/d-i/images/daily/netboot/$gtkdebian-installer/$arch"
      ${Else} ; We have only two arches, then Else means $arch == "amd64"
        StrCpy $base_url "http://d-i.debian.org/daily-images/$arch/daily/netboot/$gtkdebian-installer/$arch"
      ${Endif}
    ${ElseIf} $kernel == "kfreebsd"
      StrCpy $base_url "http://d-i.debian.org/daily-images/kfreebsd-$arch/daily/netboot/debian-installer/kfreebsd-$arch/"
    ${EndIf}
  ${Else}
    ${If} $kernel == "linux"
      StrCpy $base_url "http://ftp.se.debian.org/debian/dists/stable/main/installer-$arch/current/images/netboot/$gtkdebian-installer/$arch"
    ${ElseIf} $kernel == "kfreebsd"
      # TODO: CHECK THIS (no stable image so far)
      StrCpy $base_url "http://ftp.se.debian.org/debian/dists/stable/main/installer-kfreebsd-$arch/current/images/netboot/$gtkdebian-installer/kfreebsd-$arch"
    ${EndIf}
  ${Endif}
FunctionEnd
!endif

Function ShowDesktop
  ; It appears that "desktop" is a reserved keyword, hence the _ prefix
  Var /GLOBAL _desktop
  ReadINIStr $0 $d\win32-loader.ini "installer" "default_desktop"
  ${If} $0 == ""
    StrCpy $_desktop "gnome"
  ${Else}
    StrCpy $_desktop $0
  ${Endif}

  ; Prepare the dialog
  File /oname=$PLUGINSDIR\desktop.ini	templates/4_choices.ini
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 1" "Text" $(desktop1)
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 2" "Text" "GNOME"
  ${If} $_desktop == "gnome"
    WriteINIStr $PLUGINSDIR\desktop.ini	"Field 2" "State" "1"
  ${Endif}
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 3" "Text" "KDE"
  ${If} $_desktop == "kde"
    WriteINIStr $PLUGINSDIR\desktop.ini	"Field 2" "State" "1"
  ${Endif}
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 4" "Text" "Xfce"
  ${If} $_desktop == "xfce"
    WriteINIStr $PLUGINSDIR\desktop.ini	"Field 2" "State" "1"
  ${Endif}
  WriteINIStr $PLUGINSDIR\desktop.ini	"Field 5" "Text" $(desktop2)

  ; Display it (only in expert mode)
  ${If} $expert == true
    InstallOptions::dialog $PLUGINSDIR\desktop.ini
  ${Endif}

  ; Read from it
  ReadINIStr $0 $PLUGINSDIR\desktop.ini "Field 3" "State"
  ${If} $0 == "1"
    StrCpy $_desktop "kde"
  ${Endif}
  ReadINIStr $0 $PLUGINSDIR\desktop.ini "Field 4" "State"
  ${If} $0 == "1"
    StrCpy $_desktop "xfce"
  ${Endif}

  ; Add the results to our preseed file
  StrCpy $preseed_cfg "\
$preseed_cfg$\n\
tasksel tasksel/desktop multiselect $_desktop-desktop$\n\
tasksel tasksel/desktop seen false"
FunctionEnd

Function ShowCustom
; Gather all the missing information before ShowCustom is displayed

!ifndef NOCD

; ********************************************** Media-based install
  ${If} $kernel == "linux"
    Var /GLOBAL linux
    Var /GLOBAL initrd
    Var /GLOBAL g2ldr
    Var /GLOBAL g2ldr_mbr
    ReadINIStr $linux	$d\win32-loader.ini "installer" "$arch/$gtklinux"
    ReadINIStr $initrd	$d\win32-loader.ini "installer" "$arch/$gtkinitrd"
    ReadINIStr $g2ldr	$d\win32-loader.ini "grub" "g2ldr"
    ReadINIStr $g2ldr_mbr	$d\win32-loader.ini "grub" "g2ldr.mbr"
    StrCmp $linux		"" incomplete_ini
    StrCmp $initrd	"" incomplete_ini
    StrCmp $g2ldr		"" incomplete_ini
    StrCmp $g2ldr_mbr	"" incomplete_ini
    Goto ini_is_ok
  ${ElseIf} $kernel == "kfreebsd"
    # TODO: Check if kfreebsd will ever ship a media-based installed.
    # If so, make sure it works here.
    Goto incomplete_ini
  ${EndIf}
incomplete_ini:
  MessageBox MB_OK|MB_ICONSTOP "$(error_incomplete_ini)"
  Quit
ini_is_ok:

!endif

; ********************************************** Initialise proxy (even when in network-less mode, for the sake of preseeding)
  StrCpy $proxy ""
  ReadRegDWORD $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings" ProxyEnable
  IntCmp $0 1 0 proxyless
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Internet Settings" ProxyServer
  StrCmp $0 "" proxyless
  StrCpy $proxy "$0"
proxyless:

; ********************************************** preseed locale
  ${If} $unsupported_language == false
    ReadINIStr $0 $PLUGINSDIR\maps.ini "languages" "$LANGUAGE"
    ReadRegStr $1 HKCU "Control Panel\International" iCountry
    ReadINIStr $1 $PLUGINSDIR\maps.ini "countries" "$1"
    ${If} $0 != ""
      ${If} $1 != ""
        StrCpy $0 "$0_$1"
      ${Endif}
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i debian-installer/locale string $0"
    ${Endif}
  ${Endif}

; ********************************************** preseed domain
  systeminfo::domain
  Pop $2
  Pop $0
  ${If} $2 != 0
    ${If} $0 != ""
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i netcfg/get_domain string $0$\n\
d-i netcfg/get_domain seen false"
    ${Endif}
  ${EndIf}

; ********************************************** preseed timezone
  ReadRegStr $0 HKLM SYSTEM\CurrentControlSet\Control\TimeZoneInformation TimeZoneKeyName
  ${If} $0 == ""
    ReadRegStr $0 HKLM SYSTEM\CurrentControlSet\Control\TimeZoneInformation StandardName
  ${Endif}
  ReadINIStr $0 $PLUGINSDIR\maps.ini "timezones" "$0"
  ${If} $0 != ""
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i time/zone string $0$\n\
d-i time/zone seen false"
  ${Endif}

; ********************************************** preseed keymap
  systeminfo::keyboard_layout
  Pop $0
  ; lower word is the locale identifier (higher word is a handler to the actual layout)
  IntOp $0 $0 & 0x0000FFFF
  IntFmt $0 "0x%04X" $0
  ReadINIStr $0 $PLUGINSDIR\maps.ini "keymaps" "$0"
  ; FIXME: do we need to support non-AT keyboards here?
  ${If} $0 != ""
    ${If} $expert == true
      MessageBox MB_YESNO|MB_ICONQUESTION $(detected_keyboard_is) IDNO keyboard_bad_guess
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i console-keymaps-at/keymap select $0$\n\
d-i console-keymaps-at/keymap seen true"
      Goto keyboard_end
keyboard_bad_guess:
      MessageBox MB_OK $(keyboard_bug_report)
keyboard_end:
    ${Else}
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i console-keymaps-at/keymap select $0$\n\
d-i console-keymaps-at/keymap seen false"
    ${Endif}
  ${Endif}

; ********************************************** preseed hostname
  systeminfo::hostname
  Pop $2
  Pop $0
  ${If} $2 != 0
    ${If} $0 != ""
      StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i netcfg/get_hostname string $0$\n\
d-i netcfg/get_hostname seen false"
    ${Endif}
  ${EndIf}

; ********************************************** preseed priority
  ${If} $expert == true
    StrCpy $preseed_cmdline "$preseed_cmdline priority=low"
  ${Endif}

; ********************************************** preseed user-fullname
  systeminfo::username
  Pop $0
  ${If} $0 != ""
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i passwd/user-fullname string $0$\n\
d-i passwd/user-fullname seen false"
  ${Endif}

!ifdef NETWORK_BASE_URL
; ********************************************** if ${NETWORK_BASE_URL} provides a preseed.cfg, use it
  Push true
  Push "preseed.cfg"
  Push "$PLUGINSDIR"
  Push "${NETWORK_BASE_URL}"
  Call Download
  ${If} $0 == "success"
    StrCpy $preseed_cfg "$preseed_cfg$\n$\n"
    ClearErrors
    FileOpen $0 $PLUGINSDIR\preseed.cfg r
network_preseed_loop:
    FileRead $0 $1
    IfErrors +3
    StrCpy $preseed_cfg "\
$preseed_cfg\
$1"
    Goto network_preseed_loop
    FileClose $0
    StrCpy $preseed_cfg "$preseed_cfg$\n"
  ${Endif}
!endif

; ********************************************** Display customisation dialog now
  ${If} $expert == true
    File /oname=$PLUGINSDIR\custom.ini templates/custom.ini
    WriteINIStr $PLUGINSDIR\custom.ini "Field 1" "Text" $(custom1)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 2" "Text" $(custom2)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 3" "Text" $(custom3)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 4" "Text" "Linux cmdline:"
    WriteINIStr $PLUGINSDIR\custom.ini "Field 5" "Text" $(custom5)
    WriteINIStr $PLUGINSDIR\custom.ini "Field 6" "State" "$proxy"
!ifdef NOCD
    WriteINIStr $PLUGINSDIR\custom.ini "Field 8" "State" "$base_url"
!endif
    WriteINIStr $PLUGINSDIR\custom.ini "Field 9" "State" "$preseed_cmdline"
    InstallOptions::dialog $PLUGINSDIR\custom.ini
    ReadINIStr $proxy		$PLUGINSDIR\custom.ini "Field 6" "State"
!ifdef NOCD
    ReadINIStr $base_url	$PLUGINSDIR\custom.ini "Field 8" "State"
!endif
    ReadINIStr $preseed_cmdline	$PLUGINSDIR\custom.ini "Field 9" "State"
  ${Endif}

; do this inmediately after custom.ini, because proxy settings can be
; overriden there
; ********************************************** preseed proxy
  ${If} $proxy == ""
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i mirror/http/proxy seen true$\n"
  ${Else}
    StrCpy $preseed_cfg "\
$preseed_cfg$\n\
d-i mirror/http/proxy string http://$proxy/$\n"
  ${Endif}
FunctionEnd

Section "Installer Loader"

; ******************************************************************************
; ***************************************** THIS IS WHERE THE REAL ACTION STARTS
; ******************************************************************************

  ; Up to this point, we haven't modified host system.  The first modification
  ; we want to do is preparing the Uninstaller (so that in case something went
  ; wrong, our half-install can be undone).
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debian-Installer Loader" "DisplayName" $(program_name)
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debian-Installer Loader" "UninstallString" "$INSTDIR\uninstall.exe"

!ifndef NOCD
  ${If} $kernel == "linux"
    ClearErrors
    StrCpy $0 "$EXEDIR\$linux"
    StrCpy $1 "$INSTDIR\linux"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
    StrCpy $0 "$EXEDIR\$initrd"
    StrCpy $1 "$INSTDIR\initrd.gz"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
  ${ElseIf} $kernel == "kfreebsd"
    # TODO: Check if kfreebsd will ever ship a media-based installed.
    # If so, make sure it works here.
    MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
    Quit
  ${EndIf}
!else
  ${If} $kernel == "linux"
    Push "false"
    Push "linux"
    Push "$INSTDIR"
    Push "$base_url"
    Call Download
    Push "false"
    Push "initrd.gz"
    Push "$INSTDIR"
    Push "$base_url"
    Call Download
  ${ElseIf} $kernel == "kfreebsd"
    Push "false"
    Push "kfreebsd.gz"
    Push "$INSTDIR"
    Push "$base_url"
    Call Download
    Push "false"
    Push "initrd.gz"
    Push "$INSTDIR"
    Push "$base_url"
    Call Download
  ${EndIf}
!endif

; We're about to write down our preseed line.  This would be a nice place
; to add post-install parameters.
  StrCpy $preseed_cmdline "$preseed_cmdline --"

; ********************************************** preseed quietness
  ${If} $expert == false
    StrCpy $preseed_cmdline "$preseed_cmdline quiet"
  ${Endif}

; ********************************************** grub.cfg
  StrCpy $0 "$c\win32-loader\grub.cfg"
  DetailPrint "$(generating)"
  FileOpen $0 $c\win32-loader\grub.cfg w
  ${If} $kernel == "linux"
    FileWrite $0 "\
linux	/win32-loader/linux $preseed_cmdline$\n\
initrd	/win32-loader/initrd.gz$\n\
boot$\n"
  ${ElseIf} $kernel == "kfreebsd"
    FileWrite $0 "\
kfreebsd	/win32-loader/kfreebsd.gz$\n\
kfreebsd_module	/win32-loader/initrd.gz type=mfs_root$\n\
set kFreeBSD.vfs.root.mountfrom=ufs:/dev/md0$\n\
set kFreeBSD.hw.ata.ata_dma=0   # needed for qemu hard disk # TODO: delete$\n\
set kFreeBSD.hw.ata.atapi_dma=0 # needed for qemu cd # TODO: 1$\n\
boot$\n"
  ${EndIf}
  FileClose $0

; ********************************************** cpio hack
  File /oname=$INSTDIR\cpio.exe /usr/share/win32/cpio.exe
  File /oname=$INSTDIR\gzip.exe /usr/share/win32/gzip.exe

  StrCpy $0 "$INSTDIR\initrd.gz"
  DetailPrint "$(appending_preseeding)"

  FileOpen $0 $INSTDIR\preseed.cfg w
  FileWrite $0 "$preseed_cfg$\n"
  FileClose $0

  ; cpio awkward CLI, meet Winf**k awkward CLI
  FileOpen $0 $INSTDIR\cpio_list w
  FileWrite $0 "preseed.cfg"
  FileClose $0

  ; IMPORTANT!!  All files accessed by this script must be in the same
  ; filesystem as the script itself, because cmd.exe/command.com gets
  ; confused when using absolute paths.  This is why $INSTDIR is used
  ; instead of $PLUGINSDIR.
  FileOpen $0 $INSTDIR\cpio.bat w
  FileWrite $0 "\
cpio.exe -o -H newc < cpio_list > newc_chunk$\r$\n\
attrib -r initrd.gz$\r$\n\
gzip.exe -1 < newc_chunk >> initrd.gz$\r$\n\
"
  FileClose $0
; TODO: FIX THIS FOR kFreeBSD
${If} $kernel == "linux"
  nsExec::Exec '"$INSTDIR\cpio.bat"'
  Pop $0
  ${If} $0 != 0
    StrCpy $0 "$INSTDIR\cpio.bat"
    MessageBox MB_OK|MB_ICONSTOP "$(error_exec)"
    Quit
  ${Endif}
${EndIf}

; ********************************************** Do bootloader last, because it's the most dangerous
  ${If} $windows_boot_method == ntldr
!ifdef NOCD
    !ifdef NETWORK_BASE_URL
        Push "false"
        Push "g2ldr"
        Push "$c"
        Push "${NETWORK_BASE_URL}"
        Call Download
        Push "false"
        Push "g2ldr.mbr"
        Push "$c"
        Push "${NETWORK_BASE_URL}"
        Call Download
    !else
       File /oname=$c\g2ldr g2ldr
       File /oname=$c\g2ldr.mbr g2ldr.mbr
    !endif
!else
    ClearErrors
    StrCpy $0 "$EXEDIR\$g2ldr"
    StrCpy $1 "$c\g2ldr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
    StrCpy $0 "$EXEDIR\$g2ldr_mbr"
    StrCpy $1 "$c\g2ldr.mbr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
!endif
    DetailPrint "$(registering_ntldr)"
    SetFileAttributes "$c\boot.ini" NORMAL
    SetFileAttributes "$c\boot.ini" SYSTEM|HIDDEN

    ; Sometimes timeout isn't set.  This may result in ntldr booting straight to
    ; Windows (bad) or straight to Debian-Installer (also bad)!  Force it to 30
    ; just in case. Store its eventual old value alongside
    ; Read the already defined timeout
    ReadIniStr $0 "$c\boot.ini" "boot loader" "timeout"
    IfErrors 0 no_boot_ini_timeout
       ClearErrors
       WriteIniStr "$c\boot.ini" "boot loader" "old_timeout_win32-loader" $0
    no_boot_ini_timeout:
    WriteIniStr "$c\boot.ini" "boot loader" "timeout" "30"
    WriteIniStr "$c\boot.ini" "operating systems" "$c\g2ldr.mbr" '"$(d-i_ntldr)"'
  ${Endif}

  ${If} $windows_boot_method == direct
    File /oname=$INSTDIR\loadlin.exe loadlin.exe
    File /oname=$INSTDIR\loadlin.pif loadlin.pif
  ${Endif}

  ${If} $windows_boot_method == bootmgr
!ifdef NOCD
    !ifdef NETWORK_BASE_URL
        Push "false"
        Push "g2ldr"
        Push "$c"
        Push "${NETWORK_BASE_URL}"
        Call Download
        Push "false"
        Push "g2ldr.mbr"
        Push "$c"
        Push "${NETWORK_BASE_URL}"
        Call Download
    !else
       File /oname=$c\g2ldr g2ldr
       File /oname=$c\g2ldr.mbr g2ldr.mbr
    !endif
!else
    ClearErrors
    StrCpy $0 "$EXEDIR\$g2ldr"
    StrCpy $1 "$c\g2ldr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
    StrCpy $0 "$EXEDIR\$g2ldr_mbr"
    StrCpy $1 "$c\g2ldr.mbr"
    CopyFiles /FILESONLY "$0" "$1"
    IfErrors 0 +3
      MessageBox MB_OK|MB_ICONSTOP "$(error_copyfiles)"
      Quit
!endif
    ; FIXME: this blurb is duplicated in the uninstaller.  keep in sync!
    GetFullPathName $bcdedit $WINDIR\Sysnative\bcdedit.exe
    ${If} $bcdedit == ""
      StrCpy $bcdedit $SYSDIR\bcdedit.exe
    ${Endif}

    DetailPrint "$(registering_bootmgr)"
    ReadRegStr $0 HKLM "Software\Debian\Debian-Installer Loader" "bootmgr"
    ${If} $0 == ""
      nsExec::ExecToStack '"$bcdedit" /create /d "$(d-i)" /application bootsector'
      Pop $0
      ${If} $0 != 0
        StrCpy $0 $bcdedit
        MessageBox MB_OK|MB_ICONSTOP "$(error_exec)"
        Quit
      ${Endif}
      ; "The entry {id} was successfully created" is in top of stack now
      string::bcdedit_extract_id
      Pop $0
      ${If} $0 == "error"
        MessageBox MB_OK|MB_ICONSTOP "$(error_bcdedit_extract_id)"
        Quit
      ${Endif}
      ; $0 holds the boot id.  Write it down, both for installer idempotency
      ; and for uninstaller.
      WriteRegStr HKLM "Software\Debian\Debian-Installer Loader" "bootmgr" "$0"
    ${Endif}
    nsExec::Exec '"$bcdedit" /set $0 device boot'
    nsExec::Exec '"$bcdedit" /set $0 path \g2ldr.mbr'
    nsExec::Exec '"$bcdedit" /displayorder $0 /addlast'
  ${Endif}

; ********************************************** Needed for systems with compressed NTFS
  DetailPrint "$(disabling_ntfs_compression)"
  nsExec::Exec '"compact" /u $c\g2ldr $c\g2ldr.mbr $c\grub.cfg'
  ; in my tests, uncompressing $c\grub.cfg wasn't necessary, but better be safe than sorry
  ${If} $kernel == "linux"
    nsExec::Exec '"compact" /u $INSTDIR\linux $INSTDIR\initrd.gz'
  ${ElseIf} $kernel == "kfreebsd"
    nsExec::Exec '"compact" /u $INSTDIR\kfreebsd.gz $INSTDIR\initrd.gz'
  ${EndIf}

SectionEnd

Function .onInstSuccess
  Var /GLOBAL warning2
  ${If} $windows_boot_method == direct
    StrCpy $warning2 $(warning2_direct)
  ${Else}
    StrCpy $warning2 $(warning2_reboot)
  ${Endif}
  MessageBox MB_OK '$(warning1)$warning2$(warning3)'
  ${If} $windows_boot_method == direct
    Exec '"$INSTDIR\loadlin.pif" linux $preseed_cmdline initrd=initrd.gz'
  ${Else}
    MessageBox MB_YESNO|MB_ICONQUESTION $(reboot_now) IDNO +2
    Reboot
  ${Endif}
FunctionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  ; Initialise $c
  ReadRegStr $c HKLM "Software\Debian\Debian-Installer Loader" "system_drive"

  SetFileAttributes "$c\boot.ini" NORMAL
  SetFileAttributes "$c\boot.ini" SYSTEM|HIDDEN
  DeleteINIStr "$c\boot.ini" "operating systems" "$c\g2ldr.mbr"
    
  ReadIniStr $0 "$c\boot.ini" "boot loader" "old_timeout_win32-loader"
  IfErrors 0 no_saved_boot_ini_timeout
     ; Restore original timeout
     ClearErrors
     WriteIniStr "$c\boot.ini" "boot loader" "timeout" $0
     DeleteINIStr "$c\boot.ini" "boot loader" "old_timeout_win32-loader"
  no_saved_boot_ini_timeout:

  ReadRegStr $0 HKLM "Software\Debian\Debian-Installer Loader" "bootmgr"
  ${If} $0 != ""
    ; FIXME: this blurb is duplicated in the installer.  keep in sync!
    GetFullPathName $bcdedit $WINDIR\Sysnative\bcdedit.exe
    ${If} $bcdedit == ""
      StrCpy $bcdedit $SYSDIR\bcdedit.exe
    ${Endif}

    nsExec::Exec '"$bcdedit" /delete $0'
    Pop $0
    ${If} $0 != 0
      StrCpy $0 $bcdedit
      MessageBox MB_OK|MB_ICONSTOP "$(error_exec)"
    ${Endif}
  ${Endif}

  DeleteRegKey HKLM "Software\Debian\Debian-Installer Loader"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debian-Installer Loader"
  Delete $c\g2ldr
  Delete $c\g2ldr.mbr
  Delete $c\grub.cfg
  RMDir /r /REBOOTOK $INSTDIR
SectionEnd
