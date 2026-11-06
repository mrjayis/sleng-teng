#pragma once
// Teng-40 sound engine. Plain C++17, no framework dependency, so it can be
// driven by the plugin wrapper or by the offline render tool.

#include <array>
#include <cstdint>

#include "Patterns.h"
#include "Tones.h"
#include "Upd931.h"

namespace teng {

struct Params {
    // melody section
    int tone = 0;
    bool vibrato = false;
    bool sustain = false;
    bool velocitySensitive = false;
    int transpose = 0;
    float melodyVol = 0.7f;

    // rhythm / bass section
    int rhythm = 0;
    bool rhythmOn = false;
    double tempo = 82.0;
    bool synchro = true;   // bass key starts the rhythm (internal clock only)
    bool autoBass = true;  // bass key plays the pattern's bass line
    int bassOctave = 0;
    int splitNote = 48;    // MIDI notes below this go to the bass section
    float bassVol = 0.8f;
    float drumVol = 0.7f;

    // output
    float lofi = 0.0f;  // extra degradation on top of the chip model
    bool speaker = false;
    float masterVol = 0.8f;
};

struct Transport {
    bool useHost = false;   // follow host tempo + transport
    bool hostPlaying = false;
    double ppq = 0.0;       // host position (quarter notes) at block start
    double bpm = 120.0;
};

// ---------------------------------------------------------------------------
// Building blocks
// ---------------------------------------------------------------------------

struct Noise {
    uint32_t s = 0x12345678u;
    float next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float)(int32_t)s * (1.0f / 2147483648.0f);
    }
};

struct PulseOsc {
    double phase = 0.0;
    double inc = 0.0;
    float duty = 0.5f;
    float next();
};

struct SVF {
    float ic1 = 0, ic2 = 0;
    float g = 0, k = 1.414f;
    float a1 = 0, a2 = 0, a3 = 0;
    void set(float cutoffHz, float q, double sr);
    struct Out { float lp, bp, hp; };
    Out process(float x);
    void reset() { ic1 = ic2 = 0; }
};

// ---------------------------------------------------------------------------
// Voices
// ---------------------------------------------------------------------------

// Bass section. On the MT-40 the accompaniment CPU produces the bass tone as a
// square wave on a port pin, plus envelope trigger lines into an analog VCA.
// Modelled as: square wave -> envelope (fast attack, slow decay while gated,
// quick release) -> RC low-pass -> AC coupling.
struct BassVoice {
    PulseOsc osc;
    float env = 0.0f;
    bool gate = false;
    bool attack = false;
    int note = -1;
    float atkCoef = 0, decCoef = 0, relCoef = 0, lpA = 0;
    float lp1 = 0, lp2 = 0, dcX = 0, dcY = 0;
    void prepare(double sr);
    void start(int midiNote, double sr);
    void release() { gate = false; }
    float render();
};

struct DrumKit {
    // kick
    double kPhase = 0; float kT = 1e9f, kVel = 0;
    // snare
    float sT = 1e9f, sVel = 0; double sPhase = 0; SVF sFilt;
    // hats (closed + open share one voice; closed chokes open)
    float hT = 1e9f, hVel = 0, hDecay = 0.03f; SVF hFilt;
    // rim / clave
    float rT = 1e9f, rVel = 0; double rPhase = 0;
    Noise noise;

    void prepare(double sr);
    void kick(float v)   { kT = 0; kVel = v; kPhase = 0; }
    void snare(float v)  { sT = 0; sVel = v; sPhase = 0; }
    void hat(float v)    { hT = 0; hVel = v; hDecay = 0.025f; }
    void openHat(float v){ hT = 0; hVel = v; hDecay = 0.18f; }
    void rim(float v)    { rT = 0; rVel = v; rPhase = 0; }
    float render(double sr);
};

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

class Engine {
public:
    void prepare(double sampleRate);
    void setParams(const Params& p);
    const Params& params() const { return p_; }

    void noteOn(int note, float velocity, int channel);
    void noteOff(int note, int channel);
    void allNotesOff();
    void triggerFill() { fillRequested_ = true; }

    // Call once per host block before process(); sets the transport.
    void beginBlock(const Transport& t);
    // Render n mono samples (added to nothing: out is overwritten).
    void process(float* out, int n);

    // For the UI.
    bool rhythmRunning() const { return running_; }
    int currentStep() const { return lastStep_; }
    int bassRoot() const { return bassRoot_; }

private:
    void onBassKeyDown(int note);
    void onBassKeyUp(int note);
    void startRhythm(double beat);
    void stopRhythm();
    void tickStep(long long stepIndex);
    int heldBassKey() const;

    double sr_ = 44100.0;
    Params p_;
    Transport t_;

    Upd931 chip_;
    int heldMelody_[128] {}; // played (transposed) note per MIDI key, -1 = none

    BassVoice bass_;
    DrumKit drums_;

    // bass keyboard state
    std::array<bool, 128> bassHeld_ {};
    int bassRoot_ = -1;          // -1 = no key pressed yet
    int bassGate_ = 0;           // steps left on the current auto-bass note
    int manualBassNote_ = -1;

    // rhythm clock
    bool running_ = false;
    bool armed_ = false;         // waiting for a bass key (synchro start)
    bool prevRhythmOn_ = false;
    double beat_ = 0.0;          // position in quarter notes
    double beatOrigin_ = 0.0;    // beat at which the internal clock started
    long long lastStepIndex_ = -1;
    int lastStep_ = -1;
    int lastRhythm_ = -1;
    bool fillRequested_ = false;
    long long fillBar_ = -1;

    // output chain
    float gMel_ = 0, gBass_ = 0, gDrum_ = 0, gMaster_ = 0;
    SVF spkHP_, spkLP_, outLP_;
    float holdValue_ = 0, holdPhase_ = 1;
};

} // namespace teng
