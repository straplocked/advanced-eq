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
