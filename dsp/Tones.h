#pragma once
// The 22 melody presets, expressed as uPD931 patches (see Upd931.h).
//
// What is real: the synthesis model (delta-coded 16-step waves A/B, cycle
// masks, mirror/invert, the attack1/attack2/decay1/decay2 envelope and its
// rate tables, sustain/release rates, retrigger), taken from MAME's
// documentation of the chip.
//
// What is NOT real: the per-tone wave data and flag settings. Those live in
// the MT-40's ROM, which has not been published. Each patch below was voiced
// by ear using the chip's own building blocks. The tone names are the usual
// Casiotone set and may not match the MT-40 panel exactly.

#include <array>

#include "Upd931.h"

namespace teng {

namespace waves {
// Deltas must be 0, +-1, +-2, +-4 or +-8 and sum to zero over the 16 steps.
// The running total must stay inside the chip's 6-bit range (-32..31).
constexpr Wave931 zero    { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
constexpr Wave931 square  { 8,8,8,4, 0,0,0,0, -8,-8,-8,-4, 0,0,0,0 };     // ~50% pulse
constexpr Wave931 pulse25 { 8,8,8,4, 0,0,-8,-8, -8,-4,0,0, 0,0,0,0 };     // ~25% pulse
constexpr Wave931 narrow  { 8,8,-8,-8, 0,0,0,0, 0,0,0,0, 0,0,0,0 };       // ~12% pulse
constexpr Wave931 saw     { 2,2,2,2, 2,2,2,2, 2,2,2,2, -8,-8,-8,0 };      // ramp up, fast drop
constexpr Wave931 tri     { 4,4,4,4, 4,4,2,0, -2,-4,-4,-4, -4,-4,-4,0 };
constexpr Wave931 sine    { 1,2,4,4, 4,2,1,0, -1,-2,-4,-4, -4,-2,-1,0 };  // raised cosine
constexpr Wave931 square2 { 8,8,0,0, -8,-8,0,0, 8,8,0,0, -8,-8,0,0 };     // octave up
constexpr Wave931 sine2   { 2,4,4,2, -2,-4,-4,-2, 2,4,4,2, -2,-4,-4,-2 }; // octave up, smooth
} // namespace waves

inline constexpr int kNumTones = 22;

// clang-format off
//  name            waveA           waveB           mA mB mirr inv   a1 a2 d1  d2Q    d2Slow d2Off  fadeA  fadeB  retrig sh oct  filter  vib
inline constexpr std::array<Patch931, kNumTones> kTones {{
  { "Piano",        waves::saw,     waves::square2, 0, 0, false,false, 0, 3, 3, false, true,  false, false, true,  false, 0, 0,  3200, false },
  { "Harpsichord",  waves::narrow,  waves::square2, 0, 0, false,false, 0, 0, 4, false, false, false, false, false, false, 0, 0,  6000, false },
  { "Organ",        waves::square,  waves::square2, 0, 0, false,false, 0, 0, 7, false, false, true,  false, false, false, 0, 0,  4000, false },
  { "Pipe Organ",   waves::sine,    waves::square2, 0, 0, false,false, 4, 0, 7, false, false, true,  false, false, false, 0, 0,  3000, false },
  { "Accordion",    waves::pulse25, waves::saw,     0, 1, false,false, 3, 0, 7, false, false, true,  false, false, false, 0, 0,  3500, true  },
  { "Flute",        waves::sine,    waves::narrow,  0, 0, false,false, 3, 4, 7, false, false, true,  false, true,  false, 0, 0,  2200, true  },
  { "Clarinet",     waves::square,  waves::zero,    0, 0, false,false, 2, 0, 7, false, false, true,  false, false, false, 0, 0,  1800, false },
  { "Oboe",         waves::pulse25, waves::narrow,  0, 0, false,false, 2, 0, 7, false, false, true,  false, false, false, 0, 0,  2600, true  },
  { "Saxophone",    waves::saw,     waves::pulse25, 0, 1, false,false, 2, 0, 7, false, false, true,  false, false, false, 0, 0,  2200, true  },
  { "Trumpet",      waves::saw,     waves::narrow,  0, 0, false,false, 3, 3, 7, false, false, true,  false, true,  false, 0, 0,  2400, false },
  { "Horn",         waves::sine,    waves::square,  0, 1, false,false, 4, 0, 7, false, false, true,  false, false, false, 0, 0,  1200, false },
  { "Violin",       waves::saw,     waves::tri,     0, 0, false,false, 5, 0, 7, false, false, true,  false, false, false, 0, 0,  3500, true  },
  { "Cello",        waves::saw,     waves::square,  0, 1, false,false, 5, 0, 7, false, false, true,  false, false, false, 0, -1, 2200, true  },
  { "Celesta",      waves::sine,    waves::square2, 0, 0, false,false, 0, 4, 4, false, true,  false, false, true,  false, 0, 1,  5000, false },
  { "Vibraphone",   waves::sine,    waves::sine2,   0, 0, false,false, 0, 0, 4, false, true,  false, false, false, false, 0, 0,  2500, true  },
  { "Glockenspiel", waves::square2, waves::sine,    0, 0, false,false, 0, 0, 3, false, true,  false, false, false, false, 0, 1,  7000, false },
  { "Xylophone",    waves::sine,    waves::narrow,  0, 0, false,false, 0, 3, 2, false, false, true,  false, true,  false, 0, 1,  4000, false },
  { "Guitar",       waves::pulse25, waves::square2, 0, 0, false,false, 0, 4, 4, false, true,  false, false, true,  false, 0, 0,  3000, false },
  { "Banjo",        waves::narrow,  waves::saw,     0, 0, false,false, 0, 0, 3, false, false, false, false, false, false, 0, 0,  5000, false },
  { "Mandolin",     waves::narrow,  waves::square2, 0, 0, false,false, 0, 0, 3, false, false, false, false, false, true,  0, 0,  4500, false },
  { "Synth Reed",   waves::pulse25, waves::saw,     0, 1, false,true,  1, 0, 7, false, false, true,  false, false, false, 0, 0,  2500, true  },
  { "Fantasy",      waves::sine,    waves::square2, 0, 3, false,false, 5, 6, 5, false, true,  false, true,  false, false, 0, 0,  3000, true  },
}};
// clang-format on

} // namespace teng
