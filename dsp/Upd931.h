#pragma once
// Model of the NEC/Casio uPD931 tone generator used in Casio's early-80s
// Casiotones. This is an independent implementation written from the
// behaviour documented in MAME's upd931 device (BSD-3-Clause, Devin Acker),
// which in turn is based on Robin Whittle's reverse-engineering notes on the
// Casio MT-65. Rate tables and flag semantics follow that documentation.
//
// The chip's sound, in short:
//   * 8 voices. Each voice plays two 16-step waveforms, "A" (main) and "B"
//     (sub). A waveform is stored as 16 4-bit *delta* codes; the output is a
//     6-bit accumulator that wraps, so tones are built from slopes, not
//     sample values.
//   * Per-waveform cycle masks (play every cycle / every other cycle / one in
//     four / two in four) plus mirror/invert on alternate cycles. These give
//     the chip its octave-down and hollow, reedy timbres.
//   * A 4-stage envelope (attack1, attack2, decay1, decay2) plus release.
//     During attack2 wave A can fade in and/or wave B fade out, which is how
//     Casio did "consonant -> vowel" attacks.
//   * Everything runs at clock/8 (~618 kHz with the 4.946864 MHz crystal used
//     in MAME's CT-8000 driver) and is output through a resistor DAC and RC
//     filters.

#include <array>
#include <cstdint>

namespace teng {

// Allowed waveform deltas (the chip's 4-bit codes map onto these values).
// 0, +-1, +-2, +-4, +-8.
using Wave931 = std::array<int8_t, 16>;

struct Patch931 {
    const char* name;
    Wave931 waveA, waveB;
    uint8_t modeA, modeB;    // 0 = every cycle, 1 = on/off, 2 = 1 in 4, 3 = 2 in 4
    bool mirror, invert;     // applied on alternate cycles
    uint8_t attack1;         // 0 = instant ... 7 = slowest
    uint8_t attack2;         // 0 = instant ... 7 = slowest
    uint8_t decay1;          // 0 = fastest ... 6 = very slow, 7 = no decay (sustain)
    bool decay2AtQuarter;    // decay1 -> decay2 at 1/4 level (else 1/2)
    bool decay2Slow;         // decay2 rate 3 (else 6)
    bool decay2Disable;
    bool attack2FadeInA;     // wave A silent in attack1, fades in during attack2
    bool attack2FadeOutB;    // wave B fades out during attack2
    bool retrigger;          // re-attack every ~80 ms while decaying (mandolin)
    uint8_t envShift;        // 0..3: envelope speed x1, x2, x4, x8
    int8_t octave;           // octave offset applied by the keyboard CPU
    float filterHz;          // tone-dependent analog low-pass after the DAC
    bool vibrato;
};

class Upd931 {
public:
    static constexpr int kVoices = 8;
    static constexpr double kClock = 4946864.0;
    static constexpr double kNativeRate = kClock / 8.0;

    void prepare(double hostRate);
    void setPatch(const Patch931& p) { patch_ = &p; }
    void setSustain(bool on) { sustain_ = on; }
    void setVibrato(bool on) { vibratoSwitch_ = on; }

    void noteOn(int midiNote, float gain);
    void noteOff(int midiNote);
    void allNotesOff();

    float render(); // one host-rate sample

private:
    enum EnvState : uint8_t { Idle, Attack1, Attack2, Decay1, Decay2, Release };

    struct Voice {
        int note = -1;
        bool held = false;
        uint64_t age = 0;
        float gain = 1.0f;
        uint32_t pitch = 0;        // phase increment, 15 fractional bits per wave step
        uint32_t counter = 0;
        uint8_t wavePos = 0xff;
        int8_t out[2] {};
        EnvState env = Idle;
        uint32_t envCounter = 0;
        uint32_t level[2] {};
        uint32_t retrigCount = 0;
    };

    void startVoice(Voice& v, int note, float gain);
    void updateEnv(Voice& v);
    void updateWave(Voice& v, uint32_t pitch);

    const Patch931* patch_ = nullptr;
    std::array<Voice, kVoices> voices_ {};
    uint64_t stamp_ = 0;
    bool sustain_ = false;
    bool vibratoSwitch_ = false;

    double hostRate_ = 44100.0;
    double stepAcc_ = 0.0;
    double vibPhase_ = 0.0;

    // output stage: AC coupling + 2 RC poles
    float dcX_ = 0, dcY_ = 0, lp1_ = 0, lp2_ = 0;
};

} // namespace teng
