#include "Upd931.h"

#include <algorithm>
#include <cmath>

namespace teng {

namespace {
constexpr unsigned kPitchShift = 15;
constexpr unsigned kVolumeShift = 15;
constexpr uint32_t kVolumeMax = 0xffu << kVolumeShift;
constexpr uint32_t kRetrigSamples = 0x60000 / 8; // retrigger period in native samples (~79 ms)

constexpr uint16_t kAttack1Rates[] = { 0, 2048, 512, 256, 160, 80, 32, 8 };
constexpr uint32_t kAttack2Rates[] = { 0, 2048, 256, 128, 64, 32, 16, 8 };
constexpr uint32_t kDecay1Rates[]  = { 2048, 640, 160, 32, 16, 8, 2, 0 };
constexpr uint8_t kCycleMask[]     = { 0xf, 0x5, 0x1, 0x3 };

inline int8_t sext6(int v) { v &= 0x3f; return (int8_t)(v >= 0x20 ? v - 0x40 : v); }
} // namespace

void Upd931::prepare(double hostRate) {
    hostRate_ = hostRate;
    stepAcc_ = 0.0;
    for (auto& v : voices_) v = Voice {};
    dcX_ = dcY_ = lp1_ = lp2_ = 0.0f;
}

void Upd931::startVoice(Voice& v, int note, float gain) {
    v.note = note;
    v.held = true;
    v.age = ++stamp_;
    v.gain = gain;
    const int played = note + 12 * (patch_ ? patch_->octave : 0);
    const double freq = 440.0 * std::pow(2.0, (played - 69) / 12.0);
    v.pitch = (uint32_t)std::lround((double)(1u << kPitchShift) * freq * 16.0 / kNativeRate);
    v.counter = 0;
    v.wavePos = 0xff;
    v.out[0] = v.out[1] = 0;
    v.env = Attack1;
    v.envCounter = 0;
    v.retrigCount = 0;
}

void Upd931::noteOn(int midiNote, float gain) {
    Voice* target = nullptr;
    for (auto& v : voices_) if (v.env != Idle && v.note == midiNote) { target = &v; break; }
    if (!target) for (auto& v : voices_) if (v.env == Idle) { target = &v; break; }
    if (!target) {
        for (auto& v : voices_) if (!v.held && (!target || v.age < target->age)) target = &v;
        if (!target) for (auto& v : voices_) if (!target || v.age < target->age) target = &v;
    }
    startVoice(*target, midiNote, gain);
}

void Upd931::noteOff(int midiNote) {
    for (auto& v : voices_)
        if (v.held && v.note == midiNote) {
            v.held = false;
            if (v.env != Idle) v.env = Release;
        }
}

void Upd931::allNotesOff() {
    for (auto& v : voices_) { v.held = false; if (v.env != Idle) v.env = Release; }
}

void Upd931::updateEnv(Voice& v) {
    const Patch931& p = *patch_;
    const unsigned shift = p.envShift & 3;

    switch (v.env) {
        case Idle: return;

        case Attack1: {
            uint32_t rate;
            if (p.attack1 == 0) rate = kVolumeMax;
            else if (p.attack1 < 4 && v.envCounter >= (0xe0u << kVolumeShift)) rate = 160u << shift;
            else rate = (uint32_t)kAttack1Rates[p.attack1 & 7] << shift;
            v.envCounter = std::min(v.envCounter + rate, kVolumeMax);
            v.level[0] = p.attack2FadeInA ? 0 : v.envCounter;
            v.level[1] = v.envCounter;
            if (v.envCounter >= kVolumeMax) { v.envCounter = 0; v.env = Attack2; }
            break;
        }

        case Attack2: {
            const uint32_t rate = p.attack2 == 0 ? kVolumeMax : (kAttack2Rates[p.attack2 & 7] << shift);
            v.envCounter = std::min(v.envCounter + rate, kVolumeMax);
            if (p.attack2FadeInA) v.level[0] = v.envCounter;
            if (p.attack2FadeOutB) v.level[1] = kVolumeMax - v.envCounter;
            if (v.envCounter >= kVolumeMax) v.env = Decay1;
            break;
        }

        case Decay1: {
            const uint32_t rate = kDecay1Rates[p.decay1 & 7] << shift;
            if (v.envCounter < rate) { v.envCounter = 0; v.env = Idle; }
            else v.envCounter -= rate;
            v.level[0] = v.envCounter;
            if (v.level[1]) v.level[1] = v.envCounter;
            if (!p.decay2Disable) {
                const uint32_t lvl = p.decay2AtQuarter ? 0x40u : 0x80u;
                if (v.env != Idle && v.envCounter < (lvl << kVolumeShift)) v.env = Decay2;
            }
            break;
        }

        case Decay2: {
            const uint32_t rate = (p.decay2Slow ? 3u : 6u) << shift;
            if (v.envCounter < rate) { v.envCounter = 0; v.env = Idle; }
            else v.envCounter -= rate;
            v.level[0] = v.envCounter;
            if (v.level[1]) v.level[1] = v.envCounter;
            break;
        }

        case Release: {
            const uint32_t rate = (sustain_ ? 16u : 512u) << shift;
            if (v.envCounter < rate) { v.envCounter = 0; v.env = Idle; }
            else v.envCounter -= rate;
            v.level[0] = std::min(v.level[0], v.envCounter);
            v.level[1] = std::min(v.level[1], v.envCounter);
            break;
        }
    }

    if (p.retrigger && (v.env == Decay1 || v.env == Decay2) && ++v.retrigCount >= kRetrigSamples) {
        v.retrigCount = 0;
        v.env = Attack1;
        v.envCounter = 0;
    }
}

void Upd931::updateWave(Voice& v, uint32_t pitch) {
    const Patch931& p = *patch_;
    v.counter += pitch;
    const unsigned cycle = (v.counter >> (kPitchShift + 4)) & 3;
    unsigned pos = (v.counter >> kPitchShift) & 0xf;
    if (pos == v.wavePos) return;
    v.wavePos = (uint8_t)pos;

    if (p.mirror && (cycle & 1)) pos ^= 0xf;

    const uint8_t modes[2] = { p.modeA, p.modeB };
    const Wave931* waves[2] = { &p.waveA, &p.waveB };
    for (int i = 0; i < 2; ++i) {
        if (!((kCycleMask[modes[i] & 3] >> cycle) & 1)) continue;
        int step = (*waves[i])[pos];
        if (p.invert && (cycle & 1)) step = -step;
        v.out[i] = sext6(v.out[i] + step);
    }
}

float Upd931::render() {
    if (!patch_) return 0.0f;

    stepAcc_ += kNativeRate / hostRate_;
    const int steps = (int)stepAcc_;
    stepAcc_ -= steps;

    vibPhase_ += 6.0 / hostRate_;
    if (vibPhase_ >= 1.0) vibPhase_ -= 1.0;
    const bool vib = vibratoSwitch_ || patch_->vibrato;
    const double vibRatio = vib ? 1.0 + 0.006 * std::sin(6.283185307179586 * vibPhase_) : 1.0;

    float sum = 0.0f;
    for (auto& v : voices_) {
        if (v.env == Idle) continue;
        const uint32_t pitch = (uint32_t)(v.pitch * vibRatio);
        int32_t acc = 0;
        for (int s = 0; s < steps; ++s) {
            updateEnv(v);
            updateWave(v, pitch);
            acc += v.out[0] * (int32_t)(v.level[0] >> kVolumeShift)
                 + v.out[1] * (int32_t)(v.level[1] >> kVolumeShift);
            if (v.env == Idle) break;
        }
        sum += v.gain * (float)acc;
    }
    // average over the native samples (boxcar decimation), normalise to +-1 per waveform
    float x = steps > 0 ? sum / ((float)steps * 32.0f * 255.0f) : 0.0f;

    // AC coupling capacitor
    const float y = x - dcX_ + 0.9995f * dcY_;
    dcX_ = x;
    dcY_ = y;

    // two RC low-pass poles at the tone's filter frequency
    const float a = 1.0f - std::exp(-6.2831853f * patch_->filterHz / (float)hostRate_);
    lp1_ += a * (y - lp1_);
    lp2_ += a * (lp1_ - lp2_);
    return lp2_ * 0.5f;
}

} // namespace teng
