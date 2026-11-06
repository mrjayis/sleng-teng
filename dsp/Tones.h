#pragma once
// Preset tone table for the melody section.
//
// The MT-40 has 22 preset tones built from simple digital pulse/square waves
// mixed at different octave "footages" and shaped by a fixed envelope and a
// fixed low-pass filter. We model each preset that way: up to four pulse
// oscillators (16', 8', 4', 2'), each with its own duty cycle and level, a
// keytracked 2-pole low-pass with a decaying filter envelope, and an ADSR.
//
// The names and voicings below are approximations by ear of the classic
// Casiotone palette. They are not measured from a hardware unit; tweak the
// numbers here to taste.

#include <array>

namespace teng {

struct Footage {
    float level; // 0 = unused
    float duty;  // pulse width 0..1 (0.5 = square)
};

struct ToneDef {
    const char* name;
    Footage f16, f8, f4, f2;
    float detuneCents; // applied to the 4' and 2' footages for a slight chorus
    float cutoffHz;    // base low-pass cutoff at middle C
    float keytrack;    // 0..1, how much the cutoff follows pitch
    float envOctaves;  // filter-envelope depth in octaves
    float filterDecayMs;
    float attackMs, decayMs, sustain, releaseMs;
    bool vibrato;      // built-in vibrato (the Vibrato switch adds it to any tone)
};

inline constexpr int kNumTones = 22;

// clang-format off
inline constexpr std::array<ToneDef, kNumTones> kTones {{
    //  name            16'        8'          4'          2'         det   cut    kt    envO  fDec    A     D      S     R      vib
    { "Piano",        {0,0},     {1.0f,.50f},{.30f,.25f},{0,0},       0,  2200, .6f, 2.0f,  400,    2,  900,  0.0f,  250, false },
    { "Harpsichord",  {0,0},     {1.0f,.12f},{.50f,.25f},{0,0},       0,  4500, .5f, 1.0f,  300,    1,  700,  0.0f,  150, false },
    { "Organ",        {.5f,.5f}, {1.0f,.50f},{.60f,.50f},{.3f,.5f},   0,  3500, .3f, 0.0f,  100,    5,    0,  1.0f,   40, false },
    { "Pipe Organ",   {0,0},     {1.0f,.50f},{.80f,.50f},{.5f,.5f},   3,  6000, .3f, 0.0f,  100,   30,    0,  1.0f,  150, false },
    { "Accordion",    {0,0},     {1.0f,.30f},{.50f,.30f},{0,0},       8,  3000, .4f, 0.0f,  100,   20,    0,  1.0f,   60, true  },
    { "Flute",        {0,0},     {1.0f,.50f},{0,0},      {0,0},       0,  1200, .8f, 0.5f,  150,   60,  300,  0.9f,  120, true  },
    { "Clarinet",     {.2f,.5f}, {1.0f,.50f},{0,0},      {0,0},       0,  1600, .7f, 0.3f,  150,   30,    0,  1.0f,   80, false },
    { "Oboe",         {0,0},     {1.0f,.15f},{0,0},      {0,0},       0,  2500, .6f, 0.3f,  150,   30,    0,  1.0f,   80, true  },
    { "Saxophone",    {0,0},     {1.0f,.30f},{.30f,.50f},{0,0},       0,  2000, .6f, 1.0f,  250,   40,  400,  0.9f,  100, true  },
    { "Trumpet",      {0,0},     {1.0f,.25f},{.40f,.50f},{0,0},       0,  1800, .6f, 1.5f,  200,   40,  300,  0.9f,   80, false },
    { "Horn",         {.4f,.5f}, {.50f,.50f},{0,0},      {0,0},       0,   900, .6f, 0.5f,  250,   70,    0,  1.0f,  150, false },
    { "Violin",       {0,0},     {1.0f,.20f},{.30f,.20f},{0,0},       4,  3000, .5f, 0.0f,  100,  120,    0,  1.0f,  200, true  },
    { "Cello",        {1.0f,.2f},{.40f,.20f},{0,0},      {0,0},       0,  1800, .5f, 0.0f,  100,  100,    0,  1.0f,  200, true  },
    { "Celesta",      {0,0},     {0,0},      {1.0f,.50f},{.3f,.5f},   0,  4000, .4f, 0.5f,  200,    1,  600,  0.0f,  300, false },
    { "Vibraphone",   {0,0},     {1.0f,.50f},{0,0},      {0,0},       0,  1500, .6f, 0.5f,  300,    2, 1500,  0.0f,  600, true  },
    { "Glockenspiel", {0,0},     {0,0},      {.40f,.50f},{1.f,.5f},   0,  6000, .3f, 0.5f,  200,    1,  900,  0.0f,  500, false },
    { "Xylophone",    {0,0},     {0,0},      {1.0f,.50f},{0,0},       0,  3000, .4f, 1.0f,   80,    1,  200,  0.0f,  100, false },
    { "Guitar",       {0,0},     {1.0f,.20f},{.30f,.50f},{0,0},       0,  2500, .5f, 2.0f,  300,    2, 1200,  0.0f,  200, false },
    { "Banjo",        {0,0},     {1.0f,.10f},{.50f,.10f},{0,0},       0,  5000, .4f, 1.0f,  150,    1,  400,  0.0f,  100, false },
    { "Harp",         {0,0},     {1.0f,.50f},{.20f,.50f},{0,0},       0,  2000, .5f, 1.0f,  300,    2, 1400,  0.0f,  500, false },
    { "Synth Reed",   {.5f,.1f}, {1.0f,.10f},{0,0},      {0,0},       0,  2200, .5f, 1.0f,  300,   10,  500,  0.8f,  100, true  },
    { "Fantasy",      {0,0},     {1.0f,.50f},{.50f,.25f},{0,0},      12,  2500, .5f, 1.0f,  800,  200, 2000,  0.5f,  800, true  },
}};
// clang-format on

} // namespace teng
