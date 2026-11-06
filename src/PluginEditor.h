#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

// Retro mini-keyboard look: dark panel, cream tone tabs, orange rhythm tabs,
// vertical slide faders.
class Teng40LookAndFeel : public juce::LookAndFeel_V4 {
public:
    Teng40LookAndFeel();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;
};

// Keyboard that shades the bass section (below the split point).
class SplitKeyboard : public juce::MidiKeyboardComponent {
public:
    SplitKeyboard(juce::MidiKeyboardState& s) : MidiKeyboardComponent(s, horizontalKeyboard) {}
    int split = 48;
protected:
    void drawWhiteNote(int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                       juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote(int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                       juce::Colour noteFillColour) override;
};

class Teng40Editor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit Teng40Editor(Teng40Processor&);
    ~Teng40Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Fader {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> att;
    };
    struct Toggle {
        juce::TextButton button;
        std::unique_ptr<ButtonAttachment> att;
    };

    void setupFader(Fader&, const juce::String& paramId, const juce::String& name);
    void setupToggle(Toggle&, const juce::String& paramId, const juce::String& name, juce::Colour on);
    void setupStepper(juce::Slider&, juce::Label&, std::unique_ptr<SliderAttachment>&, const juce::String& paramId,
                      const juce::String& name);

    Teng40Processor& proc;
    Teng40LookAndFeel lnf;

    juce::OwnedArray<juce::TextButton> toneButtons, rhythmButtons;
    std::unique_ptr<juce::ParameterAttachment> toneAtt, rhythmAtt;

    Fader melodyVol, bassVol, drumVol, tempo, lofi, masterVol;
    Toggle vibrato, sustain, velocity, rhythmOn, synchro, autoBass, hostSync, speaker;
    juce::TextButton fillButton { "FILL-IN" };

    juce::Slider transpose, bassOctave, splitNote;
    juce::Label transposeLabel, bassOctaveLabel, splitLabel;
    std::unique_ptr<SliderAttachment> transposeAtt, bassOctaveAtt, splitAtt;

    SplitKeyboard keyboard;

    juce::Rectangle<int> toneArea, rhythmArea, mixArea, ledArea, brandArea;
    int ledStep = -1;
    bool ledRunning = false;
    int ledLength = 32, ledStepsPerBeat = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Teng40Editor)
};
