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
| 37-key melody keyboard, 22 preset tones, 8-voice polyphony | MIDI notes at or above the split point, played on a model of the NEC/Casio **uPD931** tone chip (see below) |
| Mini bass keyboard (15 keys) | MIDI notes below the split point (default C2 = MIDI 48), or anything on **MIDI channel 2** |
| 6 rhythms: Rock, Samba, Disco, Waltz, Swing, Pop | Same six patterns, five synthesized "analog" drum voices |
| Synchro / auto bass | **SYNCHRO** + **START/STOP** arms the rhythm, and the first bass key starts it. With **AUTO BASS** on, the bass key sets the root of the pattern's bass line |
| Bass tone (square wave from the accompaniment CPU + analog envelope) | Square wave, gated envelope (fast attack, slow decay, quick release), RC low-pass |
| Fill-in | **FILL-IN** button (and an automatable parameter) |
| Vibrato, sustain | Toggle buttons, using the chip's own release/sustain rates |

Extras: **HOST SYNC** (the rhythm follows the DAW tempo and transport), velocity
sensitivity (off by default, like the original), transpose, bass octave, a
movable split point, optional **LO-FI** degradation and a **SPEAKER** emulation.

## Getting the Sleng Teng sound

1. Pick the **ROCK** rhythm. **SYNCHRO** and **AUTO BASS** are on by default,
   and the tempo defaults to 82 BPM, the record's tempo.
2. Press **START/STOP**. Standalone, or with Host Sync off, the rhythm waits for
   a bass key. In a DAW with Host Sync on, it runs while the transport plays.
3. Hold **D** on the bass section. That's the key used on the record: on the
   MT-40 it's the second bass key from the left. The bass line follows whatever
   key you hold, so switching roots plays the riddim's changes.

## Sources and accuracy

| Part | Status | Source |
|---|---|---|
| Rock (Sleng Teng) bass line | **Supplied by the project owner** | In C, 16ths: `B C C C, B C C C, B C C C, F - E -` (B just below the root, F and E above it). Replaces an earlier version taken from Ableton Learning Music's transcription |
| Melody synthesis engine | **Modelled on the real chip** | MAME's [uPD931 device](https://github.com/mamedev/mame/blob/master/src/devices/sound/upd931.cpp) (Devin Acker, BSD-3-Clause), based on Robin Whittle's MT-65 research notes. Wave/envelope rate tables and flag behaviour follow it |
| The 22 tone presets | Approximated | The per-tone wave data is in the MT-40's ROM, which isn't published. Each preset is voiced by ear from the chip's own building blocks |
| Hardware layout (8048-family accompaniment CPU driving 5 analog drum triggers, bass tone and bass envelope triggers; melody chip with main/sub waveform outputs) | From secondary sources | Hobbyist teardown notes and forum posts found by web search; not checked against a service manual |
| Rock drums | **Supplied by the project owner** | Hi-hat on 8ths, snare on 2 and 4, kick `k..kk...` in 8ths |
| Other five rhythms (drums and bass) | Approximated | Stored in the accompaniment CPU's 1 KB internal ROM; no public dump |
| Drum and bass circuit details | Approximated | No schematic was reachable |

**Not yet confirmed:** that the MT-40 uses the uPD931 itself rather than a
close relative. Sources say its melody hardware matches the MT-31's and has
main/sub waveform DAC outputs, which is how the uPD931 works.

**What would make it exact:**

- The **MT-40 / MT-41 service manual**: schematics for the drum and bass
  circuits, filter values and clock.
- A **ROM dump** of the accompaniment CPU (NEC D8048C-316), which would give
  every rhythm and bass pattern note-for-note.
- A dump of the main CPU's ROM, which would give the 22 real tone patches.

Pattern data is in `dsp/Patterns.h`. Tone patches are in `dsp/Tones.h`, as
uPD931 register settings (wave deltas, cycle modes, envelope codes).

## Building

Requirements: CMake 3.22+, a C++17 compiler, and on Linux the usual JUCE
dependencies (ALSA, X11, freetype and fontconfig dev packages). JUCE is fetched
automatically.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release        # renders build/teng40-demo.wav
./build/teng_render disco.wav 118 Disco "Synth Reed"   # any rhythm / tone by name
```

Plugins land in `build/Teng40_artefacts/Release/{VST3,AU,Standalone}`. Copy the
VST3 to your plugin folder (`~/Library/Audio/Plug-Ins/VST3`,
`C:\Program Files\Common Files\VST3`, or `~/.vst3`).

GitHub Actions (`.github/workflows/build.yml`) builds macOS (universal), Windows
and Linux on every push and uploads the plugins as downloadable artifacts.

## Layout

```
dsp/      framework-free C++ engine: uPD931 chip model, tones, patterns, bass, drums, rhythm clock
src/      JUCE processor (parameters, MIDI, host sync) and editor (UI)
tools/    teng_render: offline smoke test that renders a demo WAV
```
