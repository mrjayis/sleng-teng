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

void Env::set(double sr, float aMs, float dMs, float sus, float rMs) {
    aInc = (float)(1.0 / (std::max(aMs, 0.5f) * 0.001 * sr));
    dCoef = dMs <= 0.0f ? 0.0f : (float)std::exp(-1.0 / (dMs * 0.001 * sr / 3.0));
    rCoef = (float)std::exp(-1.0 / (std::max(rMs, 1.0f) * 0.001 * sr / 3.0));
    s = sus;
}

float Env::next() {
    switch (stage) {
        case Off: v = 0.0f; break;
        case Attack:
            v += aInc;
            if (v >= 1.0f) { v = 1.0f; stage = Decay; }
            break;
        case Decay:
            v = s + (v - s) * dCoef;
            if (std::fabs(v - s) < 1e-4f) { v = s; stage = s <= 0.0f ? Off : Sustain; }
            break;
        case Sustain: v = s; break;
        case Release:
            v *= rCoef;
            if (v < 1e-4f) { v = 0.0f; stage = Off; }
            break;
    }
    return v;
}

// ---------------------------------------------------------------------------

void MelodyVoice::start(int midiNote, float velocity, const ToneDef& t, double sr, bool sustainOn, uint64_t stamp) {
    tone = &t;
    note = midiNote;
    vel = velocity;
    held = true;
    age = stamp;
    freq = midiToHz(midiNote);

    const Footage* f[4] = { &t.f16, &t.f8, &t.f4, &t.f2 };
    const double base[4] = { 0.5, 1.0, 2.0, 4.0 };
    const double det = std::pow(2.0, t.detuneCents / 1200.0);
    for (int i = 0; i < 4; ++i) {
        oscLevel[i] = f[i]->level;
        oscMul[i] = base[i] * (i >= 2 ? det : 1.0);
        osc[i].duty = f[i]->duty > 0.0f ? f[i]->duty : 0.5f;
        osc[i].phase = 0.0;
    }

    float rel = sustainOn ? std::max(t.releaseMs, 1500.0f) : t.releaseMs;
    amp.set(sr, t.attackMs, t.decayMs, t.sustain, rel);
    fenv.set(sr, 1.0f, t.filterDecayMs, 0.0f, t.filterDecayMs);
    amp.on();
    fenv.on();
    ctrlCounter = 0;
}

float MelodyVoice::render(double sr, float vibratoRatio) {
    float fe = fenv.next();
    if ((ctrlCounter++ & 7) == 0) {
        float cutoff = tone->cutoffHz * (float)std::pow(freq / 261.63, (double)tone->keytrack)
                     * std::exp2(tone->envOctaves * fe);
        filter.set(cutoff, 0.707f, sr);
    }
    float sum = 0.0f, norm = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (oscLevel[i] <= 0.0f) continue;
        osc[i].inc = freq * oscMul[i] * vibratoRatio / sr;
        sum += oscLevel[i] * osc[i].next();
        norm += oscLevel[i];
    }
    if (norm > 0.0f) sum /= norm;
    return filter.process(sum).lp * amp.next() * vel;
}

// ---------------------------------------------------------------------------

void BassVoice::start(int midiNote, double sr) {
    note = midiNote;
    double f = midiToHz(midiNote);
    a.inc = f / sr; a.duty = 0.5f;
    b.inc = f / sr; b.duty = 0.25f;
    amp.set(sr, 2.0f, 700.0f, 0.4f, 60.0f);
    fenv.set(sr, 1.0f, 180.0f, 0.0f, 100.0f);
    amp.on();
    fenv.on();
    ctrlCounter = 0;
}

float BassVoice::render(double sr) {
    float fe = fenv.next();
    if ((ctrlCounter++ & 7) == 0 && note >= 0) {
        float track = (float)std::sqrt(midiToHz(note) / 65.4);
        filter.set(450.0f * track * std::exp2(2.2f * fe), 0.9f, sr);
    }
    if (!amp.active()) { filter.process(0.0f); return 0.0f; }
    float x = 0.6f * a.next() + 0.4f * b.next();
    return filter.process(x).lp * amp.next();
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
    for (auto& v : voices_) { v.amp.stage = Env::Off; v.filter.reset(); }
    bass_.amp.stage = Env::Off;
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

    MelodyVoice* target = nullptr;
    for (auto& vo : voices_) if (vo.active() && vo.note == n) { target = &vo; break; }
    if (!target) for (auto& vo : voices_) if (!vo.active()) { target = &vo; break; }
    if (!target) {
        // steal: oldest released voice first, otherwise oldest
        for (auto& vo : voices_)
            if (!vo.held && (!target || vo.age < target->age)) target = &vo;
        if (!target)
            for (auto& vo : voices_)
                if (!target || vo.age < target->age) target = &vo;
    }
    target->start(n, v, kTones[(size_t)p_.tone], sr_, p_.sustain, ++stamp_);
}

void Engine::noteOff(int note, int channel) {
    if (note < 0 || note > 127) return;
    if (bassHeld_[note] && (note < p_.splitNote || channel == 2)) { onBassKeyUp(note); return; }
    if (note < p_.splitNote) { onBassKeyUp(note); return; }
    const int n = std::clamp(note + p_.transpose, 0, 127);
    for (auto& vo : voices_)
        if (vo.held && vo.note == n) vo.release();
}

void Engine::allNotesOff() {
    for (auto& vo : voices_) vo.release();
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
    const double vibInc = 5.5 / sr_;

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

        vibPhase_ += vibInc;
        if (vibPhase_ >= 1.0) vibPhase_ -= 1.0;
        const float vib = 1.0f + 0.007f * (float)std::sin(kTwoPi * vibPhase_);

        float mel = 0.0f;
        for (auto& vo : voices_) {
            if (!vo.active()) continue;
            const bool useVib = p_.vibrato || (vo.tone && vo.tone->vibrato);
            mel += vo.render(sr_, useVib ? vib : 1.0f);
        }

        gMel_ += (p_.melodyVol - gMel_) * 0.002f;
        gBass_ += (p_.bassVol - gBass_) * 0.002f;
        gDrum_ += (p_.drumVol - gDrum_) * 0.002f;
        gMaster_ += (p_.masterVol - gMaster_) * 0.002f;

        float y = 0.3f * mel * gMel_ + 0.7f * bass_.render(sr_) * gBass_ + drums_.render(sr_) * gDrum_;

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
