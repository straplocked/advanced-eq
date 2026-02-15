# Release Pipeline Design

**Goal:** Enable easy installation and updates for users via GitHub Releases, with automated installers for macOS/Windows/Linux triggered by git tags.

**Distribution:** GitHub Releases (no infrastructure needed). Users download platform-specific installers.

**Update mechanism:** Users download the latest release from GitHub. Future option to add in-plugin update check via GitHub API.

---

## Release Flow

```
Bump version in CMakeLists.txt + CHANGELOG.md
  → Commit & push
  → git tag v0.2.0 && git push --tags
  → GitHub Actions detects tag
  → Builds all 3 platforms in parallel
  → Creates installers (DMG, NSIS exe, tar.gz)
  → Publishes GitHub Release with artifacts + changelog
```

## CI Workflow: release.yml

Separate from existing `build.yml`. Triggered on `v*` tag push.

### Jobs

1. **validate** — Ensures git tag matches PROJECT_VERSION in CMakeLists.txt. Extracts changelog section for release notes. Fails pipeline on mismatch.

2. **build-macos** (depends on validate) — Builds VST3 + AU + CLAP + Standalone on macos-14. Packages into DMG via `create-dmg`.

3. **build-windows** (depends on validate) — Builds VST3 + Standalone on windows-latest. Packages with NSIS into .exe installer.

4. **build-linux** (depends on validate) — Builds VST3 + Standalone on ubuntu-latest. Packages into .tar.gz with install script.

5. **release** (depends on all build jobs) — Creates GitHub Release, attaches all artifacts, pastes changelog as body.

### Artifact Naming

- `AdvancedEQ-{version}-macOS.dmg`
- `AdvancedEQ-{version}-Windows.exe`
- `AdvancedEQ-{version}-Linux.tar.gz`

## macOS: DMG Installer

**Tool:** `create-dmg` (brew install in CI)

**Contents:**
- Advanced EQ.vst3
- Advanced EQ.component (AU)
- Advanced EQ.clap
- Advanced EQ.app (Standalone)
- Symlink to /Library/Audio/Plug-Ins/VST3/ for drag-to-install
- README.txt with install instructions (including Gatekeeper bypass: right-click > Open)

**Signing:** Deferred. Commented-out steps in workflow for future signing/notarization. Users must bypass Gatekeeper for now.

## Windows: NSIS Installer

**Script:** `packaging/installer/windows/AdvancedEQ.nsi`

**Behavior:**
- Welcome → License (GPLv3) → Component Selection → Install → Finish
- Components (checkboxes):
  - VST3 → `C:\Program Files\Common Files\VST3\`
  - Standalone → `C:\Program Files\Advanced EQ\`
- Creates Start Menu shortcut for Standalone
- Creates uninstaller (Add/Remove Programs entry)
- Overwrites previous install cleanly
- Version substituted at build time via `/DVERSION=x.y.z`

## Linux: tar.gz Archive

**Contents:**
```
AdvancedEQ-{version}-Linux/
├── Advanced EQ.vst3/
├── AdvancedEQ (Standalone)
├── install.sh
└── README.txt
```

**install.sh:** Copies VST3 to `~/.vst3/`, Standalone to `~/.local/bin/`. No sudo required.

## Version Validation

The `validate` job:
1. Parses tag (v0.2.0 → 0.2.0)
2. Reads ADVANCED_EQ_VERSION_MAJOR/MINOR/PATCH from CMakeLists.txt
3. Fails with clear error if they don't match

## Changelog Extraction

1. Finds `## [{version}]` section in CHANGELOG.md
2. Extracts content until next version header
3. Passes to GitHub Release body

## Developer Release Checklist

1. Update version in CMakeLists.txt (MAJOR/MINOR/PATCH)
2. Move CHANGELOG.md entries from [Unreleased] to [x.y.z] with date
3. Commit: `git commit -m "Release vx.y.z"`
4. Tag: `git tag vx.y.z`
5. Push: `git push && git push --tags`
6. CI handles the rest

## Files to Create/Modify

- **Create:** `.github/workflows/release.yml`
- **Create:** `packaging/installer/windows/AdvancedEQ.nsi`
- **Create:** `packaging/installer/macos/dmg-background.png` (drag-to-install background)
- **Create:** `packaging/installer/macos/README.txt`
- **Create:** `packaging/installer/linux/install.sh`
- **Create:** `packaging/installer/linux/README.txt`
- **Existing:** `.github/workflows/build.yml` (no changes needed)
- **Existing:** `CMakeLists.txt` (no changes needed — versioning already in place)
