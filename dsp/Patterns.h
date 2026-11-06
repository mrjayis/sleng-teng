#pragma once
// Rhythm + auto-bass patterns.
//
// The MT-40 has six two-bar rhythm/bass backing patterns (Rock, Samba, Disco,
// Waltz, Swing, Pop). With SYNCHRO on and a bass key held, the bass section
// plays the pattern's bass line transposed to that key. The "Rock" pattern is
// the one that became the Sleng Teng riddim.
//
// Sources: the Rock bass line was supplied by the project owner (see
// below). The other patterns and all drum grids are best guesses: the MT-40
// stores them in the 1 KB internal ROM of its accompaniment CPU (an NEC
// 8048-family part that also drives the analog drum triggers and the bass
// tone), and no dump of that ROM is publicly available. Everything is plain
// data, so correct it here if you have the hardware or a reference recording.
//
// Drum grids: one character per step.
//   '.' = rest   'o' = soft   'x' = normal   'X' = accent
// Bass notes: { step, semitones relative to the held bass key, length in steps }

#include <array>
#include <vector>

namespace teng {

struct BassNote {
    int step;
    int semis;
    int len;
};

struct Rhythm {
    const char* name;
    int beatsPerBar;
    int stepsPerBeat; // 4 = 16ths, 3 = triplets (swing), 2 = 8ths
    int bars;
    const char* kick;
    const char* snare;
    const char* hat;   // closed hi-hat (or ride for Swing)
    const char* open;  // open hi-hat
    const char* rim;   // rim / clave
    std::vector<BassNote> bass;

    int stepsPerBar() const { return beatsPerBar * stepsPerBeat; }
    int length() const { return stepsPerBar() * bars; }
};

inline constexpr int kNumRhythms = 6;

// clang-format off
inline const std::array<Rhythm, kNumRhythms> kRhythms {{
    // ROCK — the Sleng Teng pattern.
    // Bass line, in C: B C C C | B C C C | B C C C | F - E -  (16ths)
    // i.e. -1 0 0 0 (x3), then -7, rest, -8, rest relative to the held key
    // (B, F and E all below the root). Supplied by the project owner; it
    // replaces an earlier version taken from Ableton Learning Music's
    // transcription, which had a different neighbour note and ending.
    // On the MT-40 this is the two-bar Rock preset in 8ths at roughly
    // double tempo; it is stored here as one bar of 16ths so a DAW set to the
    // record's tempo (~82 BPM) lines up.
    // The drum grid is NOT from a source: it is a best guess (kick on the
    // beat, snare on the off-beat 8ths, 16th hats) pending a ROM dump.
    { "Rock", 4, 4, 1,
      "X...X...X...X...",
      "..X...X...X...X.",
      "xoxoxoxoxoxoxoxo",
      "",
      "",
      { {0,-1,1},{1,0,1},{2,0,1},{3,0,1},
        {4,-1,1},{5,0,1},{6,0,1},{7,0,1},
        {8,-1,1},{9,0,1},{10,0,1},{11,0,1},
        {12,-7,1},{14,-8,1} } },

    { "Samba", 4, 4, 2,
      "X..xX..xX..xX..xX..xX..xX..xX..x",
      "",
      "oxoxoxoxoxoxoxoxoxoxoxoxoxoxoxox",
      "",
      "x..x..x...x..x..x..x..x...x..x..",
      { {0,0,3},{3,0,1},{4,-5,3},{7,-5,1},{8,0,3},{11,0,1},{12,-5,3},{15,-5,1},
        {16,0,3},{19,0,1},{20,-5,3},{23,-5,1},{24,0,3},{27,0,1},{28,-5,3},{31,-5,1} } },

    { "Disco", 4, 4, 2,
      "X...x...X...x...X...x...X...x...",
      "....X.......X.......X.......X...",
      "x...x...x...x...x...x...x...x...",
      "..x...x...x...x...x...x...x...x.",
      "",
      { {0,0,1},{2,12,1},{4,0,1},{6,12,1},{8,0,1},{10,12,1},{12,0,1},{14,12,1},
        {16,0,1},{18,12,1},{20,0,1},{22,12,1},{24,5,1},{26,17,1},{28,7,1},{30,19,1} } },

    // WALTZ — 3/4 in 8ths.
    { "Waltz", 3, 2, 2,
      "X.....X.....",
      "..o.o...o.o.",
      "..x.x...x.x.",
      "",
      "",
      { {0,0,4},{6,-5,4} } },

    // SWING — 4/4 on a triplet grid.
    { "Swing", 4, 3, 2,
      "o.....o.....o.....o.....",
      "",
      "X..x.xX..x.xX..x.xX..x.x",
      "",
      "...x.....x.....x.....x..",
      { {0,0,2},{3,4,2},{6,7,2},{9,9,2},{12,12,2},{15,9,2},{18,7,2},{21,4,2} } },

    { "Pop", 4, 4, 2,
      "X.....x.X.......X.....x.X.....x.",
      "....X.......X.......X.......X.x.",
      "XxoxXxoxXxoxXxoxXxoxXxoxXxoxXxox",
      "",
      "",
      { {0,0,2},{3,0,1},{6,0,2},{8,7,2},{10,10,2},{12,12,2},{14,10,2},
        {16,0,2},{19,0,1},{22,0,2},{24,5,2},{26,7,2},{28,10,2},{30,7,2} } },
}};
// clang-format on

inline float stepVelocity(const char* grid, int step) {
    if (grid == nullptr || grid[0] == '\0') return 0.0f;
    switch (grid[step]) {
        case 'o': return 0.45f;
        case 'x': return 0.75f;
        case 'X': return 1.0f;
        default:  return 0.0f;
    }
}

} // namespace teng
