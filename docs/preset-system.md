# Preset System

This document describes the Advanced EQ preset system, covering the data model, factory presets, user preset storage, preset operations (apply, save, delete, navigate), and the listener notification pattern.

## Overview

The preset system provides:

- **57 factory presets** organized into 3 categories (Instruments, Bus EQ, Mastering)
- **User presets** saved as XML files in a platform-specific application data directory
- **Navigation** via previous/next cycling through the combined preset list
- **Listener notifications** for UI synchronization when the active preset changes

**Source files**:
- `source/presets/PresetData.h` -- BandPreset and Preset data structures
- `source/presets/FactoryPresets.h` / `.cpp` -- 57 hardcoded factory presets
- `source/presets/PresetManager.h` / `.cpp` -- Load, save, navigate, notify

## Data Model

### BandPreset

Represents the configuration of a single EQ band within a preset:

```cpp
struct BandPreset
{
    int         bandIndex;          // Which band slot (0-23) this entry configures
    FilterType  type       = FilterType::Bell;
    float       frequency  = 1000.0f;   // Hz
    float       gain       = 0.0f;      // dB
    float       q          = 1.0f;
    int         slopeIndex = 1;         // 0=6dB, 1=12dB, ..., 8=Brickwall
    int         stereoIndex = 0;        // 0=Stereo, 1=Left, 2=Right, 3=Mid, 4=Side
};
```

**Index mappings**:

| slopeIndex | Value |
|-----------|-------|
| 0 | 6 dB/oct |
| 1 | 12 dB/oct |
| 2 | 18 dB/oct |
| 3 | 24 dB/oct |
| 4 | 36 dB/oct |
| 5 | 48 dB/oct |
| 6 | 72 dB/oct |
| 7 | 96 dB/oct |
| 8 | Brickwall |

| stereoIndex | Value |
|------------|-------|
| 0 | Stereo |
| 1 | Left |
| 2 | Right |
| 3 | Mid |
| 4 | Side |

### Preset

Represents a complete preset containing all band configurations and metadata:

```cpp
struct Preset
{
    juce::String                name;         // Display name
    juce::String                category;     // Category grouping (e.g., "Instruments")
    std::vector<BandPreset>     bands;        // Active band configurations
    float                       outputGain = 0.0f;   // Output gain in dB
    bool                        isFactory  = true;    // True for built-in presets
    juce::File                  sourceFile;   // Disk path (only set for user presets)
};
```

Note: The `bands` vector only contains entries for bands that should be active. Bands not listed in the vector are reset to their default inactive state when the preset is applied.

## Factory Presets

### Categories and Counts

| Category | Count | Description |
|----------|-------|-------------|
| Instruments | 32 | EQ curves for individual instruments |
| Bus EQ | 13 | Mixing bus/group EQ settings |
| Mastering | 12 | Mastering chain presets |
| **Total** | **57** | |

All factory presets are defined as hardcoded `Preset` structs in `source/presets/FactoryPresets.cpp`.

### Instrument Presets (32)

Organized by instrument family:

**Vocals (6)**:
- Vocal Clarity, Vocal Warmth, Vocal De-Mud, Vocal Air, Vocal Telephone, Vocal Radio

**Guitar (4)**:
- Acoustic Guitar, Electric Guitar Clean, Electric Guitar Crunch, Guitar DI Fix

**Bass (4)**:
- Bass Finger, Bass Pick, Bass Synth Sub, Bass DI Clean

**Drums (6)**:
- Kick Drum, Snare Drum, Hi-Hat, Toms, Overhead Mics, Room Mics

**Keys and Piano (3)**:
- Grand Piano, Electric Piano, Synth Pad

**Strings and Brass (4)**:
- Strings Ensemble, Violin / Viola, Cello / Double Bass, Brass Section

**Other (5)**:
- Drum Machine, Percussion, Organ, Harmonica, Banjo / Mandolin

### Bus EQ Presets (13)

- Drum Bus Punch, Drum Bus Thick
- Vocal Bus Glue, Vocal Bus Bright
- Guitar Bus, Bass Bus, Keys Bus, Strings Bus
- Parallel Compression Bus
- Room / Reverb Bus, Delay Bus, FX Bus Tame
- Mix Bus Gentle

### Mastering Presets (12)

- Mastering Gentle Smile, Mastering Air, Mastering Warmth
- Mastering Mid Scoop, Mastering Presence
- Mastering Low-End Tighten
- Mastering Rock, Mastering Pop, Mastering Hip-Hop
- Mastering Acoustic / Folk, Mastering EDM, Mastering Classical

### Factory Preset Example

```cpp
presets.push_back ({ "Vocal Clarity", "Instruments", {
    { 0, FT::HighPass,  80.0f,   0.0f, 0.71f, 3, 0 },   // Band 0: HP at 80 Hz, 24 dB/oct
    { 1, FT::Bell,     250.0f,  -2.5f, 1.5f,  1, 0 },   // Band 1: Bell cut at 250 Hz
    { 2, FT::Bell,    3000.0f,  +3.0f, 1.2f,  1, 0 },   // Band 2: Bell boost at 3 kHz
    { 3, FT::HighShelf,10000.0f,+2.0f, 0.71f, 1, 0 },   // Band 3: High shelf at 10 kHz
}});
```

### Loading Factory Presets

```cpp
std::vector<Preset> FactoryPresets::getAll()
{
    auto all = getInstrumentPresets();   // 32 presets
    auto bus = getBusPresets();           // 13 presets
    auto master = getMasteringPresets();  // 12 presets
    all.insert (all.end(), bus.begin(), bus.end());
    all.insert (all.end(), master.begin(), master.end());
    return all;                          // 57 total
}
```

The combined list is returned in order: Instruments first, then Bus EQ, then Mastering.

## User Presets

### Storage Location

User presets are stored as individual XML files in a platform-specific directory:

| Platform | Path |
|----------|------|
| macOS | `~/Library/Application Support/Advanced EQ/User Presets/` |
| Windows | `%APPDATA%/Advanced EQ/User Presets/` |
| Linux | `~/.config/Advanced EQ/User Presets/` |

The directory is determined by:

```cpp
juce::File PresetManager::getUserPresetsDirectory() const
{
    auto appData = juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    auto dir = appData.getChildFile ("Application Support")
                      .getChildFile ("Advanced EQ")
                      .getChildFile ("User Presets");
#else
    auto dir = appData.getChildFile ("Advanced EQ")
                      .getChildFile ("User Presets");
#endif
    return dir;
}
```

### File Format

User presets are stored as the raw XML serialization of the APVTS `ValueTree`. This captures all 298 parameters exactly. The format is the same as `getStateInformation()` produces, but written directly to a file rather than compressed to binary.

Example filename: `My Vocal EQ.xml`

### Discovery

User presets are discovered by scanning the directory for `*.xml` files:

```cpp
std::vector<Preset> PresetManager::loadUserPresetsFromDisk() const
{
    std::vector<Preset> userPresets;
    auto dir = getUserPresetsDirectory();

    if (!dir.isDirectory())
        return userPresets;

    for (auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.xml"))
    {
        Preset p;
        p.name = file.getFileNameWithoutExtension();
        p.category = "User";
        p.isFactory = false;
        p.sourceFile = file;
        userPresets.push_back (std::move (p));
    }

    // Sort alphabetically by name
    std::sort (userPresets.begin(), userPresets.end(),
        [] (const Preset& a, const Preset& b) {
            return a.name.compareIgnoreCase (b.name) < 0;
        });

    return userPresets;
}
```

Note that user presets are not parsed during discovery. Only the file name is read. The actual XML content is loaded on demand when the preset is applied.

## Preset Operations

### Apply Factory Preset

When a factory preset is applied (`isFactory == true` and no `sourceFile`):

```
1. resetAllBands()
   - For all 24 bands:
     - Set active = false
     - Set type = Bell (index 0)
     - Set frequency = 1000 Hz
     - Set gain = 0 dB
     - Set Q = 1.0
     - Set slope = 12 dB/oct (index 1)
     - Set stereo = Stereo (index 0)

2. For each BandPreset in the preset:
   - Set band active = true
   - Set band type, frequency, gain, Q, slope, stereo from BandPreset values
   - Uses setValueNotifyingHost() for each parameter

3. Set output gain from preset.outputGain

4. Update currentIndex
5. Notify listeners
```

All parameters are set via `setValueNotifyingHost()`, which:
- Atomically updates the parameter value (visible to audio thread immediately)
- Notifies the host for automation recording
- Triggers any active UI attachments to update

### Apply User Preset

When a user preset is applied (`isFactory == false` and `sourceFile` exists):

```
1. Parse XML from file: juce::XmlDocument::parse(file)
2. Convert to ValueTree: juce::ValueTree::fromXml(*xml)
3. Replace entire APVTS state: apvts.replaceState(newState)
4. Update currentIndex
5. Notify listeners
```

The key difference from factory presets: user presets restore the **entire** APVTS state (all 298 parameters simultaneously), including any parameters that factory presets don't explicitly set (like dynamic EQ settings, analyzer settings, etc.).

### Save User Preset

```cpp
bool PresetManager::saveCurrentAsPreset (const juce::String& name)
{
    // 1. Create legal filename from name
    auto safeName = juce::File::createLegalFileName (name);
    auto file = dir.getChildFile (safeName + ".xml");

    // 2. Copy current APVTS state to XML
    auto state = apvts.copyState();
    auto xml = state.createXml();

    // 3. Write XML to file
    xml->writeTo (file);

    // 4. Rebuild preset list (auto-discovers the new file)
    rebuildPresetList();

    // 5. Select the newly saved preset
    for (int i = 0; i < allPresets.size(); ++i)
    {
        if (!allPresets[i].isFactory && allPresets[i].name == safeName)
        {
            currentIndex = i;
            break;
        }
    }

    // 6. Notify listeners
    notifyListeners();
    return true;
}
```

The `createLegalFileName()` call sanitizes the name by removing characters that are invalid in file paths. The directory is created if it doesn't already exist.

### Delete User Preset

```cpp
bool PresetManager::deleteUserPreset (int presetIndex)
{
    auto& preset = allPresets[presetIndex];
    if (preset.isFactory) return false;       // Cannot delete factory presets

    preset.sourceFile.deleteFile();            // Remove from disk
    rebuildPresetList();                       // Refresh list
    notifyListeners();
    return true;
}
```

If the deleted preset was the currently active preset, `currentIndex` is set to -1. If it was before the current preset in the list, `currentIndex` is decremented to maintain the correct selection.

### Preset Navigation

**Next preset**:
```cpp
void PresetManager::nextPreset()
{
    int next = currentIndex + 1;
    if (next >= allPresets.size())
        next = 0;                   // Wrap to beginning
    applyPreset (next);
}
```

**Previous preset**:
```cpp
void PresetManager::previousPreset()
{
    int prev = currentIndex - 1;
    if (prev < 0)
        prev = allPresets.size() - 1;  // Wrap to end
    applyPreset (prev);
}
```

Navigation wraps around in both directions.

### Rescan User Presets

```cpp
void PresetManager::rescanUserPresets()
{
    auto currentName = getCurrentPresetName();
    rebuildPresetList();                      // Reload factory + user presets
    // Attempt to find and re-select the previously active preset by name
    currentIndex = -1;
    for (int i = 0; i < allPresets.size(); ++i)
    {
        if (allPresets[i].name == currentName)
        {
            currentIndex = i;
            break;
        }
    }
    notifyListeners();
}
```

## Preset List Structure

The combined preset list (`allPresets`) is ordered:

```
Index  0 - 31:  Factory Instruments (32 presets)
Index 32 - 44:  Factory Bus EQ (13 presets)
Index 45 - 56:  Factory Mastering (12 presets)
Index 57+:      User presets (alphabetically sorted by name)
```

The list is rebuilt from scratch whenever presets are added, deleted, or rescanned:

```cpp
void PresetManager::rebuildPresetList()
{
    allPresets = FactoryPresets::getAll();               // Start with 57 factory
    auto userPresets = loadUserPresetsFromDisk();         // Scan user directory
    allPresets.insert (allPresets.end(),
                       userPresets.begin(), userPresets.end());  // Append user presets
}
```

## Listener Pattern

### PresetManager::Listener Interface

```cpp
struct Listener
{
    virtual ~Listener() = default;
    virtual void presetChanged() = 0;
};
```

### Registration

```cpp
presetManager.addListener (this);      // Register
presetManager.removeListener (this);   // Unregister (in destructor)
```

### Notification

```cpp
void PresetManager::notifyListeners()
{
    listeners.call ([] (Listener& l) { l.presetChanged(); });
}
```

Uses `juce::ListenerList` for thread-safe, exception-safe listener notification.

### Consumers

The `HeaderToolbar` is the primary listener:

```cpp
class HeaderToolbar : public juce::Component,
                      public PresetManager::Listener
{
    void presetChanged() override
    {
        updatePresetName();  // Refresh the displayed preset name
    }
};
```

When a preset is applied, navigated, saved, or deleted, `notifyListeners()` is called, causing the header to update the preset name button text to show the new active preset name (or "Init" if no preset is selected).

## Preset Menu UI

The `HeaderToolbar` provides a hierarchical popup menu for preset selection:

```
[Instruments]
    Vocal Clarity          <-- checkmark if active
    Vocal Warmth
    ...
[Bus EQ]
    Drum Bus Punch
    ...
[Mastering]
    Mastering Gentle Smile
    ...
[User]
    My Custom Preset
    ...
```

Presets are organized by category with a checkmark next to the currently active preset. Clicking a preset applies it immediately.

## Preset Application Flowchart

```
User selects preset (via menu, next/prev, or direct call)
    |
    v
PresetManager::applyPreset(index)
    |
    +--[Factory preset?]
    |       |
    |       v
    |   resetAllBands()    -- Set all 24 bands to defaults
    |       |
    |       v
    |   For each BandPreset:
    |       setBandFromPreset() -- Activate and configure band
    |       |
    |       v
    |   Set output gain
    |
    +--[User preset?]
    |       |
    |       v
    |   Load XML from sourceFile
    |       |
    |       v
    |   apvts.replaceState(newState) -- Replace all 298 parameters at once
    |
    v
currentIndex = presetIndex
    |
    v
notifyListeners()
    |
    v
HeaderToolbar::presetChanged() --> updatePresetName()
    |
    v
Audio thread picks up new parameter values on next processBlock()
```
