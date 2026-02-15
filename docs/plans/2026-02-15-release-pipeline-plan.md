# Release Pipeline Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Automate plugin releases so that pushing a git tag builds installers for all platforms and publishes them to GitHub Releases.

**Architecture:** A new `release.yml` GitHub Actions workflow triggered on `v*` tags. It validates the version, builds on 3 platforms in parallel, creates platform-specific installers (DMG/NSIS/tar.gz), and publishes a GitHub Release with changelog notes.

**Tech Stack:** GitHub Actions, create-dmg (macOS), NSIS (Windows), tar/gzip (Linux), bash scripting

---

### Task 1: Create the NSIS installer script

**Files:**
- Create: `packaging/installer/windows/AdvancedEQ.nsi`

**Step 1: Write the NSIS script**

```nsi
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
```

**Step 2: Commit**

```bash
git add packaging/installer/windows/AdvancedEQ.nsi
git commit -m "feat: add NSIS installer script for Windows"
```

---

### Task 2: Create the macOS DMG README

**Files:**
- Create: `packaging/installer/macos/README.txt`

**Step 1: Write the README**

```text
Advanced EQ - Installation Instructions
========================================

To install, drag the plugin files to their destinations:

  VST3:       /Library/Audio/Plug-Ins/VST3/
  AU:         /Library/Audio/Plug-Ins/Components/
  CLAP:       /Library/Audio/Plug-Ins/CLAP/
  Standalone: /Applications/

IMPORTANT - First Launch on macOS:
Since this app is not notarized, macOS may block it.
To allow it, right-click the file and select "Open", then
click "Open" in the dialog. You only need to do this once
per file. Alternatively, run in Terminal:

  xattr -cr "/Library/Audio/Plug-Ins/VST3/Advanced EQ.vst3"
  xattr -cr "/Library/Audio/Plug-Ins/Components/Advanced EQ.component"

Then restart your DAW and rescan plugins.
```

**Step 2: Commit**

```bash
git add packaging/installer/macos/README.txt
git commit -m "feat: add macOS DMG install instructions"
```

---

### Task 3: Create the Linux install script and README

**Files:**
- Create: `packaging/installer/linux/install.sh`
- Create: `packaging/installer/linux/README.txt`

**Step 1: Write install.sh**

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

VST3_DIR="$HOME/.vst3"
BIN_DIR="$HOME/.local/bin"

echo "Installing Advanced EQ..."

# VST3
if [ -d "$SCRIPT_DIR/Advanced EQ.vst3" ]; then
    mkdir -p "$VST3_DIR"
    cp -r "$SCRIPT_DIR/Advanced EQ.vst3" "$VST3_DIR/"
    echo "  VST3 -> $VST3_DIR/Advanced EQ.vst3"
fi

# Standalone
if [ -f "$SCRIPT_DIR/AdvancedEQ" ]; then
    mkdir -p "$BIN_DIR"
    cp "$SCRIPT_DIR/AdvancedEQ" "$BIN_DIR/"
    chmod +x "$BIN_DIR/AdvancedEQ"
    echo "  Standalone -> $BIN_DIR/AdvancedEQ"
fi

echo "Done! Restart your DAW and rescan plugins."
```

**Step 2: Write README.txt**

```text
Advanced EQ - Linux Installation
=================================

Automatic install (recommended):
  chmod +x install.sh
  ./install.sh

This installs to:
  VST3:       ~/.vst3/
  Standalone:  ~/.local/bin/

Manual install:
  Copy "Advanced EQ.vst3" to ~/.vst3/ (or /usr/lib/vst3/)
  Copy "AdvancedEQ" to a directory in your PATH

Then restart your DAW and rescan plugins.
```

**Step 3: Commit**

```bash
git add packaging/installer/linux/install.sh packaging/installer/linux/README.txt
git commit -m "feat: add Linux install script and README"
```

---

### Task 4: Create the release workflow

This is the main task. The workflow handles validation, building, packaging, and publishing.

**Files:**
- Create: `.github/workflows/release.yml`

**Step 1: Write the release workflow**

```yaml
name: Release

on:
  push:
    tags: ['v*']

permissions:
  contents: write

jobs:
  # ── Validate version tag matches CMakeLists.txt ──
  validate:
    runs-on: ubuntu-latest
    outputs:
      version: ${{ steps.version.outputs.version }}
      changelog: ${{ steps.changelog.outputs.changelog }}
    steps:
      - uses: actions/checkout@v4

      - name: Extract version from tag
        id: version
        run: |
          TAG="${GITHUB_REF#refs/tags/v}"
          echo "version=$TAG" >> "$GITHUB_OUTPUT"
          echo "Extracted version: $TAG"

      - name: Validate version matches CMakeLists.txt
        run: |
          TAG_VERSION="${{ steps.version.outputs.version }}"
          MAJOR=$(grep 'ADVANCED_EQ_VERSION_MAJOR' CMakeLists.txt | head -1 | grep -o '[0-9]\+')
          MINOR=$(grep 'ADVANCED_EQ_VERSION_MINOR' CMakeLists.txt | head -1 | grep -o '[0-9]\+')
          PATCH=$(grep 'ADVANCED_EQ_VERSION_PATCH' CMakeLists.txt | head -1 | grep -o '[0-9]\+')
          CMAKE_VERSION="$MAJOR.$MINOR.$PATCH"
          echo "Tag version: $TAG_VERSION"
          echo "CMake version: $CMAKE_VERSION"
          if [ "$TAG_VERSION" != "$CMAKE_VERSION" ]; then
            echo "::error::Version mismatch! Tag is v$TAG_VERSION but CMakeLists.txt has $CMAKE_VERSION"
            exit 1
          fi

      - name: Extract changelog
        id: changelog
        run: |
          VERSION="${{ steps.version.outputs.version }}"
          # Extract section between this version header and the next version header
          CHANGELOG=$(awk "/^## \\[$VERSION\\]/{found=1; next} /^## \\[/{if(found) exit} found{print}" CHANGELOG.md)
          if [ -z "$CHANGELOG" ]; then
            CHANGELOG="No changelog entry found for version $VERSION."
          fi
          # Use GitHub Actions delimiter for multiline output
          EOF=$(dd if=/dev/urandom bs=15 count=1 status=none | base64)
          echo "changelog<<$EOF" >> "$GITHUB_OUTPUT"
          echo "$CHANGELOG" >> "$GITHUB_OUTPUT"
          echo "$EOF" >> "$GITHUB_OUTPUT"

  # ── macOS: Build + DMG ──
  build-macos:
    needs: validate
    runs-on: macos-14
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release -j

      # TODO: Add code signing step here when Apple Developer account is ready
      # - name: Sign plugins
      #   run: ...
      # - name: Notarize
      #   run: ...

      - name: Install create-dmg
        run: brew install create-dmg

      - name: Stage DMG contents
        run: |
          mkdir -p dmg-stage
          cp -R "build/AdvancedEQ_artefacts/Release/VST3/Advanced EQ.vst3" dmg-stage/
          cp -R "build/AdvancedEQ_artefacts/Release/AU/Advanced EQ.component" dmg-stage/
          cp -R "build/AdvancedEQ_artefacts/Release/Standalone/Advanced EQ.app" dmg-stage/
          if [ -d "build/AdvancedEQ_artefacts/Release/CLAP" ]; then
            cp -R "build/AdvancedEQ_artefacts/Release/CLAP/Advanced EQ.clap" dmg-stage/
          fi
          cp packaging/installer/macos/README.txt dmg-stage/

      - name: Create DMG
        run: |
          VERSION="${{ needs.validate.outputs.version }}"
          create-dmg \
            --volname "Advanced EQ $VERSION" \
            --window-size 600 400 \
            --icon-size 80 \
            --text-size 14 \
            --hide-extension "Advanced EQ.vst3" \
            --hide-extension "Advanced EQ.component" \
            --hide-extension "Advanced EQ.app" \
            "AdvancedEQ-$VERSION-macOS.dmg" \
            dmg-stage/

      - name: Upload DMG
        uses: actions/upload-artifact@v4
        with:
          name: macos-dmg
          path: AdvancedEQ-*.dmg

  # ── Windows: Build + NSIS ──
  build-windows:
    needs: validate
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Configure
        run: cmake -B build -G "Visual Studio 17 2022"

      - name: Build
        run: cmake --build build --config Release

      - name: Build NSIS installer
        run: |
          $version = "${{ needs.validate.outputs.version }}"
          $buildDir = "${{ github.workspace }}\build"
          & "C:\Program Files (x86)\NSIS\makensis.exe" /DVERSION=$version /DBUILD_DIR=$buildDir packaging\installer\windows\AdvancedEQ.nsi
        shell: pwsh

      - name: Upload installer
        uses: actions/upload-artifact@v4
        with:
          name: windows-installer
          path: packaging/installer/windows/AdvancedEQ-*.exe

  # ── Linux: Build + tar.gz ──
  build-linux:
    needs: validate
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Dependencies
        run: >
          sudo apt-get update && sudo apt-get install -y
          libasound2-dev
          libfreetype6-dev
          libx11-dev
          libxrandr-dev
          libxcursor-dev
          libxinerama-dev
          libgl1-mesa-dev
          libxcomposite-dev

      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release -j

      - name: Package tar.gz
        run: |
          VERSION="${{ needs.validate.outputs.version }}"
          STAGING="AdvancedEQ-$VERSION-Linux"
          mkdir -p "$STAGING"
          cp -r build/AdvancedEQ_artefacts/Release/VST3/*.vst3 "$STAGING/" 2>/dev/null || true
          # Standalone path varies — find it
          find build/AdvancedEQ_artefacts -name "Advanced EQ" -type f -executable -exec cp {} "$STAGING/AdvancedEQ" \; 2>/dev/null || true
          cp packaging/installer/linux/install.sh "$STAGING/"
          cp packaging/installer/linux/README.txt "$STAGING/"
          chmod +x "$STAGING/install.sh"
          tar czf "AdvancedEQ-$VERSION-Linux.tar.gz" "$STAGING"

      - name: Upload archive
        uses: actions/upload-artifact@v4
        with:
          name: linux-archive
          path: AdvancedEQ-*-Linux.tar.gz

  # ── Publish GitHub Release ──
  release:
    needs: [validate, build-macos, build-windows, build-linux]
    runs-on: ubuntu-latest
    steps:
      - name: Download all artifacts
        uses: actions/download-artifact@v4
        with:
          path: artifacts

      - name: Create GitHub Release
        uses: softprops/action-gh-release@v2
        with:
          tag_name: ${{ github.ref_name }}
          name: Advanced EQ v${{ needs.validate.outputs.version }}
          body: |
            ## What's Changed

            ${{ needs.validate.outputs.changelog }}

            ## Installation

            **macOS:** Download the `.dmg`, open it, and drag the plugin files to your Audio/Plug-Ins folders. See the README inside for Gatekeeper instructions.

            **Windows:** Download and run the `.exe` installer. It will install the VST3 plugin and optionally the standalone app.

            **Linux:** Download the `.tar.gz`, extract it, and run `./install.sh` — or manually copy the files (see README inside).
          files: |
            artifacts/macos-dmg/*.dmg
            artifacts/windows-installer/*.exe
            artifacts/linux-archive/*.tar.gz
          draft: false
          prerelease: false
```

**Step 2: Commit**

```bash
git add .github/workflows/release.yml
git commit -m "feat: add tag-triggered release workflow with installers"
```

---

### Task 5: Test the workflow locally (dry run)

This task validates the scripts before pushing.

**Step 1: Validate the NSIS script syntax**

This can only be fully tested in CI on Windows, but verify the file is well-formed:

Run: `head -5 packaging/installer/windows/AdvancedEQ.nsi`
Expected: The `!include "MUI2.nsh"` line and other NSIS directives

**Step 2: Validate the release workflow YAML**

Run: `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/release.yml'))"`
Expected: No errors (valid YAML)

**Step 3: Validate the install.sh script**

Run: `bash -n packaging/installer/linux/install.sh`
Expected: No syntax errors

**Step 4: Validate version extraction logic**

Run: `MAJOR=$(grep 'ADVANCED_EQ_VERSION_MAJOR' CMakeLists.txt | head -1 | grep -o '[0-9]\+') && MINOR=$(grep 'ADVANCED_EQ_VERSION_MINOR' CMakeLists.txt | head -1 | grep -o '[0-9]\+') && PATCH=$(grep 'ADVANCED_EQ_VERSION_PATCH' CMakeLists.txt | head -1 | grep -o '[0-9]\+') && echo "$MAJOR.$MINOR.$PATCH"`
Expected: `0.1.0`

**Step 5: Validate changelog extraction logic**

Run: `awk '/^## \[0.1.0\]/{found=1; next} /^## \[/{if(found) exit} found{print}' CHANGELOG.md`
Expected: The Added section content from 0.1.0

**Step 6: Commit (no changes — validation only)**

No commit needed. If any validation fails, fix the issue in the relevant file and re-run.

---

### Task 6: Update README with release/install instructions

**Files:**
- Modify: `README.md`

**Step 1: Add an Installation section to the README**

Add the following after the existing feature list / before the "Building from Source" section. The exact insertion point depends on the current README structure — find the build instructions and add this above them:

```markdown
## Installation

Download the latest release for your platform from the [Releases page](https://github.com/straplocked/advanced-eq/releases).

### macOS
1. Download `AdvancedEQ-x.y.z-macOS.dmg`
2. Open the DMG and drag plugin files to:
   - **VST3** → `/Library/Audio/Plug-Ins/VST3/`
   - **AU** → `/Library/Audio/Plug-Ins/Components/`
   - **CLAP** → `/Library/Audio/Plug-Ins/CLAP/`
3. On first launch, right-click → Open to bypass Gatekeeper (unsigned builds)
4. Restart your DAW and rescan plugins

### Windows
1. Download `AdvancedEQ-x.y.z-Windows.exe`
2. Run the installer — it places the VST3 plugin in the standard location
3. Restart your DAW and rescan plugins

### Linux
1. Download `AdvancedEQ-x.y.z-Linux.tar.gz`
2. Extract and run `./install.sh` (installs to `~/.vst3/` and `~/.local/bin/`)
3. Restart your DAW and rescan plugins
```

**Step 2: Commit**

```bash
git add README.md
git commit -m "docs: add installation instructions for releases"
```

---

### Task 7: Final commit and summary

**Step 1: Verify all files are committed**

Run: `git status`
Expected: Clean working tree

**Step 2: Verify the full file list**

Run: `git log --oneline -6`
Expected: 5 new commits for tasks 1-4 and 6

The pipeline is ready. To perform the first release:
1. Bump version in CMakeLists.txt if desired
2. Update CHANGELOG.md
3. `git tag v0.1.0 && git push && git push --tags`
4. Watch GitHub Actions build and publish the release
