# Teng-40

A plugin recreation of the early-80s Casio MT-40 mini keyboard, the one whose
"Rock" rhythm preset became the **Sleng Teng** riddim (Wayne Smith, 1985). The
"Rock" bass/rhythm pattern was programmed by Casio engineer Okuda Hiroko.

Builds as **VST3**, **AU** (macOS) and a **Standalone** app, using JUCE 8.

> Not affiliated with or endorsed by Casio. "Casio" and "MT-40" are used only to
> describe the instrument this plugin is modelled on.

## What's modelled

| MT-40 feature | Teng-40 |
|---|---|
| 37-key melody keyboard, 22 preset tones, 8-voice polyphony | MIDI notes at or above the split point. 22 pulse-wave tones with octave footages, a fixed filter and an envelope |
| Mini bass keyboard (15 keys) | MIDI notes below the split point (default C2 = MIDI 48), or anything on **MIDI channel 2** |
| 6 rhythms: Rock, Samba, Disco, Waltz, Swing, Pop | Same six two-bar patterns with a simple synthesized drum kit |
| Synchro / auto bass | **SYNCHRO** + **START/STOP** arms the rhythm, and the first bass key starts it. With **AUTO BASS** on, the bass key sets the root of the pattern's bass line |
| Fill-in | **FILL-IN** button (and an automatable parameter) |
| Vibrato, sustain | Toggle buttons |
| Lo-fi digital tone, tiny speaker | **LO-FI** (sample-rate and bit reduction) and a **SPEAKER** emulation |

Extras: **HOST SYNC** (the rhythm follows the DAW tempo and transport), velocity
sensitivity (off by default, like the original), transpose, bass octave, and a
movable split point.

## Getting the Sleng Teng sound

1. Pick the **ROCK** rhythm and turn **SYNCHRO** and **AUTO BASS** on (both are
   the defaults).
2. Press **START/STOP**. Standalone, or with Host Sync off, the rhythm waits for
   a bass key. In a DAW with Host Sync on, it runs while the transport plays.
3. Hold a bass key below the split. The bass line follows whatever key you hold,
   so switching roots plays the riddim's changes.

## Accuracy notes — please read

The tone voicings, drum sounds and **pattern data were made by ear and from
memory**. They were not dumped from the instrument's ROM, and I didn't have a
hardware unit to compare against. The Rock bass line is close in feel but may
not match note-for-note.

Everything is plain data, so it's easy to correct:

- `dsp/Patterns.h` holds the rhythm grids (`X`/`x`/`o`/`.` per step) and the bass
  lines (`{step, semitones, length}`).
- `dsp/Tones.h` holds the 22 tone presets: footage levels, pulse widths, filter
  and envelope.

If you have an MT-40 (or a clean recording of the preset), A/B it with the
plugin and adjust these files.

## Building

Requirements: CMake 3.22+, a C++17 compiler, and on Linux the usual JUCE
dependencies (ALSA, X11, freetype and fontconfig dev packages). JUCE is fetched
automatically.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release        # renders build/teng40-demo.wav
```

Plugins land in `build/Teng40_artefacts/Release/{VST3,AU,Standalone}`. Copy the
VST3 to your plugin folder (`~/Library/Audio/Plug-Ins/VST3`,
`C:\Program Files\Common Files\VST3`, or `~/.vst3`).

GitHub Actions (`.github/workflows/build.yml`) builds macOS (universal), Windows
and Linux on every push and uploads the plugins as downloadable artifacts.

## Layout

```
dsp/      framework-free C++ engine (tones, patterns, voices, drums, rhythm clock)
src/      JUCE processor (parameters, MIDI, host sync) and editor (UI)
tools/    teng_render: offline smoke test that renders a demo WAV
```
