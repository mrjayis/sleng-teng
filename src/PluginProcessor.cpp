#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids {
constexpr const char* tone = "tone";
constexpr const char* vibrato = "vibrato";
constexpr const char* sustain = "sustain";
constexpr const char* velocity = "velocity";
constexpr const char* transpose = "transpose";
constexpr const char* melodyVol = "melodyVol";
constexpr const char* rhythm = "rhythm";
constexpr const char* rhythmOn = "rhythmOn";
constexpr const char* tempo = "tempo";
constexpr const char* hostSync = "hostSync";
constexpr const char* synchro = "synchro";
constexpr const char* autoBass = "autoBass";
constexpr const char* bassOctave = "bassOctave";
constexpr const char* splitNote = "splitNote";
constexpr const char* bassVol = "bassVol";
constexpr const char* drumVol = "drumVol";
constexpr const char* fill = "fill";
constexpr const char* lofi = "lofi";
constexpr const char* speaker = "speaker";
constexpr const char* masterVol = "masterVol";
} // namespace ids

juce::AudioProcessorValueTreeState::ParameterLayout Teng40Processor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    StringArray toneNames, rhythmNames;
    for (const auto& t : teng::kTones) toneNames.add(t.name);
    for (const auto& r : teng::kRhythms) rhythmNames.add(r.name);

    auto noteName = [](int v, int) { return MidiMessage::getMidiNoteName(v, true, true, 3); };

    l.add(std::make_unique<AudioParameterChoice>(ParameterID { ids::tone, 1 }, "Tone", toneNames, 0));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::vibrato, 1 }, "Vibrato", false));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::sustain, 1 }, "Sustain", false));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::velocity, 1 }, "Velocity Sensitive", false));
    l.add(std::make_unique<AudioParameterInt>(ParameterID { ids::transpose, 1 }, "Transpose", -12, 12, 0));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { ids::melodyVol, 1 }, "Melody Volume",
                                                NormalisableRange<float>(0.f, 1.f), 0.7f));

    l.add(std::make_unique<AudioParameterChoice>(ParameterID { ids::rhythm, 1 }, "Rhythm", rhythmNames, 0));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::rhythmOn, 1 }, "Rhythm Start/Stop", false));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { ids::tempo, 1 }, "Tempo",
                                                NormalisableRange<float>(40.f, 240.f, 0.1f), 82.f));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::hostSync, 1 }, "Host Sync", true));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::synchro, 1 }, "Synchro Start", true));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::autoBass, 1 }, "Auto Bass", true));
    l.add(std::make_unique<AudioParameterInt>(ParameterID { ids::bassOctave, 1 }, "Bass Octave", -2, 2, 0));
    l.add(std::make_unique<AudioParameterInt>(ParameterID { ids::splitNote, 1 }, "Split Point", 24, 84, 48,
                                              AudioParameterIntAttributes().withStringFromValueFunction(noteName)));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { ids::bassVol, 1 }, "Bass Volume",
                                                NormalisableRange<float>(0.f, 1.f), 0.8f));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { ids::drumVol, 1 }, "Rhythm Volume",
                                                NormalisableRange<float>(0.f, 1.f), 0.7f));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::fill, 1 }, "Fill-In", false));

    l.add(std::make_unique<AudioParameterFloat>(ParameterID { ids::lofi, 1 }, "Lo-Fi",
                                                NormalisableRange<float>(0.f, 1.f), 0.0f));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { ids::speaker, 1 }, "Speaker", false));
    l.add(std::make_unique<AudioParameterFloat>(ParameterID { ids::masterVol, 1 }, "Master Volume",
                                                NormalisableRange<float>(0.f, 1.f), 0.8f));
    return l;
}

Teng40Processor::Teng40Processor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "TENG40", createLayout()) {}

bool Teng40Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void Teng40Processor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    readParams();
    engine.prepare(sampleRate);
    monoSize = juce::jmax(samplesPerBlock, 32);
    mono.allocate((size_t)monoSize, true);
    keyboardState.reset();
}

void Teng40Processor::readParams() {
    auto f = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    teng::Params p;
    p.tone = (int)f(ids::tone);
    p.vibrato = f(ids::vibrato) > 0.5f;
    p.sustain = f(ids::sustain) > 0.5f;
    p.velocitySensitive = f(ids::velocity) > 0.5f;
    p.transpose = (int)f(ids::transpose);
    p.melodyVol = f(ids::melodyVol);
    p.rhythm = (int)f(ids::rhythm);
    p.rhythmOn = f(ids::rhythmOn) > 0.5f;
    p.tempo = f(ids::tempo);
    p.synchro = f(ids::synchro) > 0.5f;
    p.autoBass = f(ids::autoBass) > 0.5f;
    p.bassOctave = (int)f(ids::bassOctave);
    p.splitNote = (int)f(ids::splitNote);
    p.bassVol = f(ids::bassVol);
    p.drumVol = f(ids::drumVol);
    p.lofi = f(ids::lofi);
    p.speaker = f(ids::speaker) > 0.5f;
    p.masterVol = f(ids::masterVol);
    engine.setParams(p);

    const bool fillParam = f(ids::fill) > 0.5f;
    if ((fillParam && !lastFillParam) || fillRequested.exchange(false)) engine.triggerFill();
    lastFillParam = fillParam;
}

void Teng40Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n > monoSize) { monoSize = n; mono.allocate((size_t)monoSize, true); }

    readParams();

    teng::Transport t;
    const bool hostSync = apvts.getRawParameterValue(ids::hostSync)->load() > 0.5f;
    // The standalone app has no host transport, so it always uses the internal clock.
    if (hostSync && wrapperType != wrapperType_Standalone) {
        if (auto* ph = getPlayHead()) {
            if (auto pos = ph->getPosition()) {
                t.useHost = true;
                t.hostPlaying = pos->getIsPlaying();
                t.ppq = pos->getPpqPosition().orFallback(0.0);
                t.bpm = pos->getBpm().orFallback(120.0);
            }
        }
    }
    engine.beginBlock(t);

    keyboardState.processNextMidiBuffer(midi, 0, n, true);

    int done = 0;
    for (const auto meta : midi) {
        const int pos = juce::jlimit(0, n, meta.samplePosition);
        if (pos > done) { engine.process(mono.get() + done, pos - done); done = pos; }
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.noteOn(m.getNoteNumber(), m.getFloatVelocity(), m.getChannel());
        else if (m.isNoteOff()) engine.noteOff(m.getNoteNumber(), m.getChannel());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    }
    if (n > done) engine.process(mono.get() + done, n - done);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom(ch, 0, mono.get(), n);

    uiRunning.store(engine.rhythmRunning());
    uiStep.store(engine.currentStep());
    uiBassRoot.store(engine.bassRoot());
}

juce::AudioProcessorEditor* Teng40Processor::createEditor() { return new Teng40Editor(*this); }

void Teng40Processor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void Teng40Processor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Teng40Processor(); }
