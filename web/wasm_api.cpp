// C API that exposes the Teng-40 engine to the browser (WebAssembly).
// The page's AudioWorklet calls teng_process() for every 128-frame block and
// forwards notes and parameter changes from the UI thread.

#include <cstdlib>
#include <new>

#include "Engine.h"

using namespace teng;

// Exceptions are disabled in this build, so provide allocation functions that
// abort instead of throwing (the stock libc++ ones pull in exception support).
void* operator new(std::size_t n) { if (void* p = std::malloc(n ? n : 1)) return p; std::abort(); }
void* operator new[](std::size_t n) { return operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

#define TENG_EXPORT(name) extern "C" __attribute__((export_name(#name)))

namespace {
Engine g_engine;
Params g_params;
float g_out[4096];

// Keep in sync with PARAM in web/app.js.
enum ParamId {
    P_TONE = 0, P_VIBRATO, P_SUSTAIN, P_VELOCITY, P_TRANSPOSE, P_MELODY_VOL,
    P_RHYTHM, P_RHYTHM_ON, P_TEMPO, P_SYNCHRO, P_AUTO_BASS, P_BASS_OCTAVE,
    P_SPLIT, P_BASS_VOL, P_DRUM_VOL, P_LOFI, P_SPEAKER, P_MASTER_VOL,
};
} // namespace

TENG_EXPORT(teng_init) void teng_init(float sampleRate) {
    g_engine.setParams(g_params);
    g_engine.prepare(sampleRate);
}

TENG_EXPORT(teng_set_param) void teng_set_param(int id, float v) {
    Params& p = g_params;
    const bool on = v > 0.5f;
    const int i = (int)(v + (v >= 0 ? 0.5f : -0.5f));
    switch (id) {
        case P_TONE: p.tone = i; break;
        case P_VIBRATO: p.vibrato = on; break;
        case P_SUSTAIN: p.sustain = on; break;
        case P_VELOCITY: p.velocitySensitive = on; break;
        case P_TRANSPOSE: p.transpose = i; break;
        case P_MELODY_VOL: p.melodyVol = v; break;
        case P_RHYTHM: p.rhythm = i; break;
        case P_RHYTHM_ON: p.rhythmOn = on; break;
        case P_TEMPO: p.tempo = v; break;
        case P_SYNCHRO: p.synchro = on; break;
        case P_AUTO_BASS: p.autoBass = on; break;
        case P_BASS_OCTAVE: p.bassOctave = i; break;
        case P_SPLIT: p.splitNote = i; break;
        case P_BASS_VOL: p.bassVol = v; break;
        case P_DRUM_VOL: p.drumVol = v; break;
        case P_LOFI: p.lofi = v; break;
        case P_SPEAKER: p.speaker = on; break;
        case P_MASTER_VOL: p.masterVol = v; break;
        default: return;
    }
    g_engine.setParams(p);
}

TENG_EXPORT(teng_note_on) void teng_note_on(int note, float velocity, int channel) { g_engine.noteOn(note, velocity, channel); }
TENG_EXPORT(teng_note_off) void teng_note_off(int note, int channel) { g_engine.noteOff(note, channel); }
TENG_EXPORT(teng_all_notes_off) void teng_all_notes_off() { g_engine.allNotesOff(); }
TENG_EXPORT(teng_fill) void teng_fill() { g_engine.triggerFill(); }

// Renders n mono samples (n <= 4096) and returns a pointer to them.
TENG_EXPORT(teng_process) float* teng_process(int n) {
    if (n > 4096) n = 4096;
    Transport t; // the browser has no host transport: internal clock
    g_engine.beginBlock(t);
    g_engine.process(g_out, n);
    return g_out;
}

TENG_EXPORT(teng_running) int teng_running() { return g_engine.rhythmRunning() ? 1 : 0; }
TENG_EXPORT(teng_step) int teng_step() { return g_engine.currentStep(); }

// Preset tables, so the UI never duplicates them.
TENG_EXPORT(teng_num_tones) int teng_num_tones() { return kNumTones; }
TENG_EXPORT(teng_tone_name) const char* teng_tone_name(int i) { return kTones[(size_t)i].name; }
TENG_EXPORT(teng_num_rhythms) int teng_num_rhythms() { return kNumRhythms; }
TENG_EXPORT(teng_rhythm_name) const char* teng_rhythm_name(int i) { return kRhythms[(size_t)i].name; }
TENG_EXPORT(teng_rhythm_length) int teng_rhythm_length(int i) { return kRhythms[(size_t)i].length(); }
TENG_EXPORT(teng_rhythm_steps_per_beat) int teng_rhythm_steps_per_beat(int i) { return kRhythms[(size_t)i].stepsPerBeat; }
