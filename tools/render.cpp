// Offline renderer / smoke test for the Teng-40 engine.
//
//   teng_render [out.wav] [bpm]
//
// Checks the pattern tables, then renders a short Sleng Teng-style demo:
// Rock rhythm at 82 BPM with synchro auto-bass on D, then G, and organ
// stabs on top. Exits non-zero if the output is silent or not finite.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "Engine.h"

using namespace teng;

static bool checkPatterns() {
    bool ok = true;
    for (const auto& r : kRhythms) {
        const char* grids[] = { r.kick, r.snare, r.hat, r.open, r.rim };
        for (const char* g : grids) {
            size_t len = std::strlen(g);
            if (len != 0 && (int)len != r.length()) {
                std::fprintf(stderr, "pattern %s: grid length %zu, expected %d\n", r.name, len, r.length());
                ok = false;
            }
        }
        for (const auto& b : r.bass)
            if (b.step < 0 || b.step >= r.length()) {
                std::fprintf(stderr, "pattern %s: bass step %d out of range\n", r.name, b.step);
                ok = false;
            }
    }
    // uPD931 waves: legal delta codes, zero net drift, stay inside the 6-bit accumulator.
    for (const auto& t : kTones) {
        const Wave931* ws[2] = { &t.waveA, &t.waveB };
        for (const Wave931* w : ws) {
            int acc = 0, lo = 0, hi = 0;
            for (int d : *w) {
                const int a = d < 0 ? -d : d;
                if (!(a == 0 || a == 1 || a == 2 || a == 4 || a == 8)) {
                    std::fprintf(stderr, "tone %s: illegal delta %d\n", t.name, d);
                    ok = false;
                }
                acc += d;
                lo = std::min(lo, acc);
                hi = std::max(hi, acc);
            }
            if (acc != 0 || lo < -32 || hi > 31) {
                std::fprintf(stderr, "tone %s: wave drifts (sum %d, range %d..%d)\n", t.name, acc, lo, hi);
                ok = false;
            }
        }
    }
    return ok;
}

static void writeWav(const char* path, const std::vector<float>& mono, int sr) {
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::perror(path); std::exit(1); }
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    const uint32_t dataBytes = (uint32_t)mono.size() * 2;
    std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes);
    std::fwrite("WAVEfmt ", 1, 8, f); u32(16); u16(1); u16(1); u32((uint32_t)sr); u32((uint32_t)sr * 2); u16(2); u16(16);
    std::fwrite("data", 1, 4, f); u32(dataBytes);
    for (float s : mono) {
        int v = (int)std::lrint(std::fmax(-1.0f, std::fmin(1.0f, s)) * 32767.0f);
        int16_t x = (int16_t)v;
        std::fwrite(&x, 2, 1, f);
    }
    std::fclose(f);
}

int main(int argc, char** argv) {
    const char* outPath = argc > 1 ? argv[1] : "teng40-demo.wav";
    const double bpm = argc > 2 ? std::atof(argv[2]) : 82.0;
    const int sr = 44100;

    if (!checkPatterns()) return 2;

    Engine e;
    Params p;
    p.rhythm = 0;      // Rock
    p.tempo = bpm;
    p.synchro = true;
    p.autoBass = true;
    p.tone = 2;        // Organ for the stabs
    p.lofi = 0.3f;
    e.setParams(p);
    e.prepare(sr);

    // Event list in beats.
    struct Ev { double beat; bool on; int note; };
    std::vector<Ev> evs;
    // Bass key D (the key used for Sleng Teng), then G, back to D: two bars each.
    const int bassKeys[] = { 38, 38, 43, 38 };
    for (int i = 0; i < 4; ++i) {
        evs.push_back({ i * 8.0 + 0.0, true, bassKeys[i] });
        evs.push_back({ i * 8.0 + 7.9, false, bassKeys[i] });
        // off-beat organ stabs (D minor / G minor triads), from bar 3 on
        if (i == 0) continue;
        const int root = bassKeys[i] + 24;
        const int chord[3] = { root, root + 3, root + 7 };
        for (int b = 0; b < 8; ++b)
            for (int c : chord) {
                evs.push_back({ i * 8.0 + b + 0.5, true, c });
                evs.push_back({ i * 8.0 + b + 0.8, false, c });
            }
    }

    const double totalBeats = 32.0 + 2.0;
    const int total = (int)(totalBeats * 60.0 / bpm * sr);
    std::vector<float> out((size_t)total, 0.0f);

    Transport t; // internal clock
    p.rhythmOn = true;
    e.setParams(p);

    const int block = 256;
    for (int pos = 0; pos < total; pos += block) {
        const int n = std::min(block, total - pos);
        e.beginBlock(t);
        const double b0 = pos * bpm / 60.0 / sr;
        const double b1 = (pos + n) * bpm / 60.0 / sr;
        int done = 0;
        for (const auto& ev : evs) {
            if (ev.beat < b0 || ev.beat >= b1) continue;
            int off = (int)((ev.beat - b0) * 60.0 / bpm * sr);
            off = std::max(done, std::min(off, n));
            if (off > done) { e.process(out.data() + pos + done, off - done); done = off; }
            if (ev.on) e.noteOn(ev.note, 0.8f, 1); else e.noteOff(ev.note, 1);
        }
        if (n > done) e.process(out.data() + pos + done, n - done);
    }

    double sumSq = 0.0; float peak = 0.0f;
    for (float s : out) {
        if (!std::isfinite(s)) { std::fprintf(stderr, "non-finite sample\n"); return 3; }
        sumSq += (double)s * s;
        peak = std::fmax(peak, std::fabs(s));
    }
    const double rms = std::sqrt(sumSq / out.size());
    std::printf("rendered %d samples, rms %.4f, peak %.4f -> %s\n", total, rms, peak, outPath);
    if (rms < 0.01) { std::fprintf(stderr, "output too quiet\n"); return 4; }

    writeWav(outPath, out, sr);
    return 0;
}
