#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Engine.h"

class Teng40Processor : public juce::AudioProcessor {
public:
    Teng40Processor();
    ~Teng40Processor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    void requestFill() { fillRequested.store(true); }

    // UI feedback
    std::atomic<bool> uiRunning { false };
    std::atomic<int> uiStep { -1 };
    std::atomic<int> uiBassRoot { -1 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void readParams();

    teng::Engine engine;
    std::atomic<bool> fillRequested { false };
    bool lastFillParam = false;
    juce::HeapBlock<float> mono;
    int monoSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Teng40Processor)
};
