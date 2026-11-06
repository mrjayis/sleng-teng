#include "Engine.h"

#include <algorithm>
#include <cmath>

namespace teng {

namespace {
constexpr double kTwoPi = 6.283185307179586;
constexpr float kPi = 3.14159265358979f;

inline double midiToHz(double note) { return 440.0 * std::pow(2.0, (note - 69.0) / 12.0); }

// PolyBLEP residual: removes the worst aliasing from the pulse edges while
// keeping the hard, buzzy character of the original square-wave voices.
inline float polyBlep(double t, double dt) {
    if (t < dt) { t /= dt; return (float)(t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float)(t * t + t + t + 1.0); }
    return 0.0f;
}

inline long long floorDiv(long long a, long long b) {
    long long q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
    return q;
}
inline long long posMod(long long a, long long b) { long long m = a % b; return m < 0 ? m + b : m; }
} // namespace

// ---------------------------------------------------------------------------

float PulseOsc::next() {
    if (inc <= 0.0) return 0.0f;
    float v = phase < duty ? 1.0f : -1.0f;
    v += polyBlep(phase, inc);
    double t2 = phase + 1.0 - duty;
    if (t2 >= 1.0) t2 -= 1.0;
    v -= polyBlep(t2, inc);
    phase += inc;
    while (phase >= 1.0) phase -= 1.0;
    // Remove the DC offset of a non-50% pulse.
    return v - (2.0f * duty - 1.0f);
}

void SVF::set(float cutoffHz, float q, double sr) {
    float fc = std::clamp(cutoffHz, 10.0f, (float)(sr * 0.45));
    g = std::tan(kPi * fc / (float)sr);
    k = 1.0f / std::max(q, 0.05f);
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

SVF::Out SVF::process(float x) {
    float v3 = x - ic2;
    float v1 = a1 * ic1 + a2 * v3;
    float v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0f * v1 - ic1;
    ic2 = 2.0f * v2 - ic2;
    return { v2, v1, x - k * v1 - v2 };
}

// ---------------------------------------------------------------------------

void BassVoice::prepare(double sr) {
    auto coef = [sr](double ms) { return (float)std::exp(-1.0 / (ms * 0.001 * sr)); };
    atkCoef = coef(1.0);
    decCoef = coef(450.0);
    relCoef = coef(25.0);
    lpA = 1.0f - (float)std::exp(-kTwoPi * 1100.0 / sr);
    osc.duty = 0.5f;
}

void BassVoice::start(int midiNote, double sr) {
    note = midiNote;
    osc.inc = midiToHz(midiNote) / sr; // free-running, like a CPU timer output
    gate = true;
    env = std::max(env, 0.0f);
    attack = true;
}

float BassVoice::render() {
    if (gate) {
        if (attack) {
            env = 1.0f - (1.0f - env) * atkCoef;
            if (env > 0.98f) attack = false;
        } else {
            env *= decCoef;
        }
    } else {
        env *= relCoef;
    }
    float x = env > 1e-5f ? osc.next() * env : 0.0f;
    if (env <= 1e-5f) osc.next();
    lp1 += lpA * (x - lp1);
    lp2 += lpA * (lp1 - lp2);
    const float y = lp2 - dcX + 0.999f * dcY;
    dcX = lp2;
    dcY = y;
    return y;
}

// ---------------------------------------------------------------------------

void DrumKit::prepare(double sr) {
    sFilt.set(3200.0f, 0.6f, sr);
    hFilt.set(7500.0f, 0.8f, sr);
    kT = sT = hT = rT = 1e9f;
}

float DrumKit::render(double sr) {
    const float dt = (float)(1.0 / sr);
    float out = 0.0f;

    if (kT < 0.6f) {
        float f = 48.0f + 120.0f * std::exp(-kT / 0.02f);
        kPhase += f / sr;
        if (kPhase >= 1.0) kPhase -= 1.0;
        out += kVel * ((float)std::sin(kTwoPi * kPhase) * std::exp(-kT / 0.16f)
                       + 0.25f * noise.next() * std::exp(-kT / 0.002f));
        kT += dt;
    }
    if (sT < 0.6f) {
        sPhase += 190.0 / sr;
        if (sPhase >= 1.0) sPhase -= 1.0;
        auto n = sFilt.process(noise.next());
        float tone = 0.5f * (float)std::sin(kTwoPi * sPhase) * std::exp(-sT / 0.04f);
        float body = (0.6f * n.hp + 1.2f * n.bp) * std::exp(-sT / 0.11f);
        out += sVel * 0.7f * (tone + body);
        sT += dt;
    }
    if (hT < 1.0f) {
        auto n = hFilt.process(noise.next());
        out += hVel * 0.45f * n.hp * std::exp(-hT / hDecay);
        hT += dt;
    }
    if (rT < 0.2f) {
        rPhase += 1700.0 / sr;
        if (rPhase >= 1.0) rPhase -= 1.0;
        out += rVel * 0.5f * (float)std::sin(kTwoPi * rPhase) * std::exp(-rT / 0.015f);
        rT += dt;
    }
    return out * 0.8f;
}

// ---------------------------------------------------------------------------

void Engine::prepare(double sampleRate) {
    sr_ = sampleRate;
    drums_.prepare(sr_);
    chip_.prepare(sr_);
    chip_.setPatch(kTones[(size_t)p_.tone]);
    for (int& n : heldMelody_) n = -1;
    bass_ = BassVoice {};
    bass_.prepare(sr_);
    spkHP_.set(180.0f, 0.707f, sr_);
    spkLP_.set(4500.0f, 1.2f, sr_);
    outLP_.set(16000.0f, 0.707f, sr_);
    stopRhythm();
    armed_ = false;
    prevRhythmOn_ = false;
    bassHeld_.fill(false);
    gMel_ = p_.melodyVol; gBass_ = p_.bassVol; gDrum_ = p_.drumVol; gMaster_ = p_.masterVol;
}

void Engine::setParams(const Params& p) {
    p_ = p;
    p_.tone = std::clamp(p_.tone, 0, kNumTones - 1);
    p_.rhythm = std::clamp(p_.rhythm, 0, kNumRhythms - 1);
    p_.tempo = std::clamp(p_.tempo, 30.0, 300.0);
    // The tone selector rewrites the chip's patch registers immediately,
    // so changing tone also changes notes that are already sounding.
    chip_.setPatch(kTones[(size_t)p_.tone]);
    chip_.setSustain(p_.sustain);
    chip_.setVibrato(p_.vibrato);
}

void Engine::beginBlock(const Transport& t) {
    const bool modeChanged = t.useHost != t_.useHost;
    t_ = t;
    if (modeChanged) { stopRhythm(); armed_ = false; prevRhythmOn_ = false; }

    if (t_.useHost) {
        armed_ = false;
        const bool shouldRun = t_.hostPlaying && p_.rhythmOn;
        if (!shouldRun && running_) stopRhythm();
        if (shouldRun && !running_) { running_ = true; lastStepIndex_ = -1; }
        beat_ = t_.ppq;
    } else {
        if (p_.rhythmOn && !prevRhythmOn_) {
            if (p_.synchro) armed_ = true; // wait for a bass key
            else startRhythm(0.0);
        } else if (!p_.rhythmOn && prevRhythmOn_) {
            stopRhythm();
            armed_ = false;
        }
        if (!p_.synchro && armed_) { armed_ = false; startRhythm(0.0); }
    }
    prevRhythmOn_ = p_.rhythmOn;
}

void Engine::startRhythm(double beat) {
    running_ = true;
    armed_ = false;
    beat_ = beat;
    lastStepIndex_ = -1;
    fillBar_ = -1;
}

void Engine::stopRhythm() {
    if (running_ && manualBassNote_ < 0) bass_.release();
    running_ = false;
    bassGate_ = 0;
    lastStepIndex_ = -1;
    lastStep_ = -1;
    fillBar_ = -1;
    fillRequested_ = false;
}

int Engine::heldBassKey() const {
    for (int n = 0; n < 128; ++n)
        if (bassHeld_[n]) return n;
    return -1;
}

void Engine::onBassKeyDown(int note) {
    bassHeld_[note] = true;
    bassRoot_ = note;
    if (armed_) startRhythm(0.0);

    if (running_ && p_.autoBass) {
        // Pattern follows the new root from its next note on.
        return;
    }
    manualBassNote_ = note;
    bass_.start(note + 12 * p_.bassOctave, sr_);
}

void Engine::onBassKeyUp(int note) {
    bassHeld_[note] = false;
    int other = heldBassKey();
    if (other >= 0) bassRoot_ = other;
    if (manualBassNote_ == note) {
        manualBassNote_ = -1;
        bass_.release();
    }
}

void Engine::noteOn(int note, float velocity, int channel) {
    if (note < 0 || note > 127) return;
    if (note < p_.splitNote || channel == 2) { onBassKeyDown(note); return; }

    const int n = std::clamp(note + p_.transpose, 0, 127);
    const float v = p_.velocitySensitive ? std::clamp(velocity, 0.05f, 1.0f) : 0.85f;

    heldMelody_[note] = n;
    chip_.noteOn(n, v);
}

void Engine::noteOff(int note, int channel) {
    if (note < 0 || note > 127) return;
    if (bassHeld_[note] && (note < p_.splitNote || channel == 2)) { onBassKeyUp(note); return; }
    if (note < p_.splitNote) { onBassKeyUp(note); return; }
    if (heldMelody_[note] >= 0) {
        chip_.noteOff(heldMelody_[note]);
        heldMelody_[note] = -1;
    }
}

void Engine::allNotesOff() {
    chip_.allNotesOff();
    for (int& n : heldMelody_) n = -1;
    bassHeld_.fill(false);
    manualBassNote_ = -1;
    bass_.release();
}

void Engine::tickStep(long long idx) {
    const Rhythm& r = kRhythms[(size_t)p_.rhythm];
    const int len = r.length();
    const int spBar = r.stepsPerBar();
    const int step = (int)posMod(idx, len);
    const long long bar = floorDiv(idx, spBar);
    const int inBar = (int)posMod(idx, spBar);
    lastStep_ = step;

    if (fillRequested_) {
        fillRequested_ = false;
        fillBar_ = (spBar - inBar > r.stepsPerBeat) ? bar : bar + 1;
    }

    if (bar == fillBar_) {
        // Fill-in: snare roll that thickens towards the end of the bar.
        if (inBar == 0) drums_.kick(1.0f);
        const bool secondHalf = inBar >= spBar / 2;
        if (secondHalf || (inBar % 2 == 0)) {
            float ramp = 0.5f + 0.5f * (float)inBar / (float)(spBar - 1);
            drums_.snare(ramp);
        }
    } else {
        if (float v = stepVelocity(r.kick, step); v > 0) drums_.kick(v);
        if (float v = stepVelocity(r.snare, step); v > 0) drums_.snare(v);
        if (float v = stepVelocity(r.hat, step); v > 0) drums_.hat(v);
        if (float v = stepVelocity(r.open, step); v > 0) drums_.openHat(v);
        if (float v = stepVelocity(r.rim, step); v > 0) drums_.rim(v);
    }

    if (p_.autoBass && bassRoot_ >= 0 && manualBassNote_ < 0) {
        if (bassGate_ > 0 && --bassGate_ == 0) bass_.release();
        for (const auto& bn : r.bass) {
            if (bn.step != step) continue;
            bass_.start(std::clamp(bassRoot_ + bn.semis + 12 * p_.bassOctave, 0, 127), sr_);
            bassGate_ = std::max(1, bn.len);
        }
    }
}

void Engine::process(float* out, int n) {
    const Rhythm& r = kRhythms[(size_t)p_.rhythm];
    const double bpm = t_.useHost ? t_.bpm : p_.tempo;
    const double beatInc = std::max(1.0, bpm) / 60.0 / sr_;

    // Output-chain settings (cheap to recompute per block).
    const float lofi = std::clamp(p_.lofi, 0.0f, 1.0f);
    const double holdInc = (1.0 - 0.75 * lofi); // relative rate: 1 = full rate
    const float steps = std::exp2(15.0f - 9.0f * lofi);
    outLP_.set(18000.0f - 11000.0f * lofi, 0.707f, sr_);

    for (int i = 0; i < n; ++i) {
        if (running_) {
            long long idx = (long long)std::floor(beat_ * r.stepsPerBeat + 1e-9);
            if (idx != lastStepIndex_) { lastStepIndex_ = idx; tickStep(idx); }
            beat_ += beatInc;
        }

        const float mel = chip_.render();

        gMel_ += (p_.melodyVol - gMel_) * 0.002f;
        gBass_ += (p_.bassVol - gBass_) * 0.002f;
        gDrum_ += (p_.drumVol - gDrum_) * 0.002f;
        gMaster_ += (p_.masterVol - gMaster_) * 0.002f;

        float y = 0.9f * mel * gMel_ + 0.7f * bass_.render() * gBass_ + drums_.render(sr_) * gDrum_;

        if (p_.speaker) {
            y = spkHP_.process(y).hp;
            y = spkLP_.process(y).lp * 1.4f;
        }

        if (lofi > 0.0f) {
            holdPhase_ += (float)holdInc;
            if (holdPhase_ >= 1.0f) {
                holdPhase_ -= 1.0f;
                holdValue_ = std::round(y * steps) / steps;
            }
            y = outLP_.process(holdValue_).lp;
        }

        out[i] = std::tanh(y * 1.2f) * gMaster_;
    }
}

} // namespace teng
