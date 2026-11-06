#include "PluginEditor.h"

namespace colours {
const juce::Colour panel { 0xff1d1d1f };
const juce::Colour panelHi { 0xff2b2b2e };
const juce::Colour silver { 0xffb9bcc0 };
const juce::Colour text { 0xffe8e4d8 };
const juce::Colour dimText { 0xff8f8b80 };
const juce::Colour cream { 0xffe9e2cc };
const juce::Colour orange { 0xffe8742a };
const juce::Colour red { 0xffd8322e };
const juce::Colour blue { 0xff4a90c8 };
const juce::Colour ledOff { 0xff3a1410 };
const juce::Colour ledOn { 0xffff3b2a };
} // namespace colours

// ---------------------------------------------------------------------------

Teng40LookAndFeel::Teng40LookAndFeel() {
    setColour(juce::Label::textColourId, colours::text);
    setColour(juce::Slider::textBoxTextColourId, colours::text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, colours::panelHi);
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xff2a2620));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff1a1000));
}

void Teng40LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over,
                                             bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(1.5f);
    const auto base = b.findColour(juce::TextButton::buttonColourId);
    const bool on = b.getToggleState();
    auto c = on ? b.findColour(juce::TextButton::buttonOnColourId) : base;
    if (down) c = c.darker(0.25f);
    else if (over) c = c.brighter(0.08f);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(r.translated(0, 1.5f), 3.0f);
    g.setGradientFill(juce::ColourGradient(c.brighter(0.15f), r.getX(), r.getY(), c.darker(0.15f), r.getX(),
                                           r.getBottom(), false));
    g.fillRoundedRectangle(r, 3.0f);
    if (on) {
        g.setColour(colours::ledOn.withAlpha(0.9f));
        g.fillEllipse(r.getRight() - 8.0f, r.getY() + 3.0f, 5.0f, 5.0f);
    }
}

void Teng40LookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    g.setColour(b.findColour(b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId));
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    auto area = b.getLocalBounds().reduced(3, 2);
    const auto lines = juce::StringArray::fromLines(b.getButtonText());
    if (lines.size() < 2) {
        g.drawFittedText(b.getButtonText(), area, juce::Justification::centred, 2, 0.6f);
        return;
    }
    // Tone tabs: number on top, name below. Long single words are squeezed
    // horizontally rather than truncated.
    const int lineH = 12;
    const int nameLines = lines[1].containsChar(' ') ? 2 : 1;
    auto block = area.withSizeKeepingCentre(area.getWidth(), lineH * (1 + nameLines));
    g.drawText(lines[0], block.removeFromTop(lineH), juce::Justification::centred, false);
    g.drawFittedText(lines[1], block, juce::Justification::centredTop, nameLines, 0.4f);
}

void Teng40LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                         juce::Slider::SliderStyle style, juce::Slider& s) {
    if (style != juce::Slider::LinearVertical) {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, pos, 0, 0, style, s);
        return;
    }
    const float cx = (float)x + (float)w * 0.5f;
    // slot
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(cx - 3.0f, (float)y, 6.0f, (float)h, 3.0f);
    // tick marks
    g.setColour(colours::dimText.withAlpha(0.6f));
    for (int i = 0; i <= 10; ++i) {
        const float ty = (float)y + (float)h * (float)i / 10.0f;
        g.drawHorizontalLine((int)ty, cx - 12.0f, cx - 7.0f);
        g.drawHorizontalLine((int)ty, cx + 7.0f, cx + 12.0f);
    }
    // cap
    juce::Rectangle<float> cap(cx - 11.0f, pos - 7.0f, 22.0f, 14.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(cap.translated(0, 2), 2.0f);
    g.setGradientFill(juce::ColourGradient(colours::silver.brighter(0.3f), cap.getX(), cap.getY(),
                                           colours::silver.darker(0.4f), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawHorizontalLine((int)pos, cap.getX() + 2, cap.getRight() - 2);
}

// ---------------------------------------------------------------------------

void SplitKeyboard::drawWhiteNote(int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                  juce::Colour lineColour, juce::Colour textColour) {
    auto c = note < split ? juce::Colour(0xffd6dce4) : juce::Colour(0xfff4f0e6);
    if (isOver) c = c.darker(0.05f);
    if (isDown) c = note < split ? colours::blue : colours::orange;
    g.setColour(c);
    g.fillRect(area);
    g.setColour(lineColour);
    g.fillRect(area.withLeft(area.getRight() - 1.0f));
    if (note % 12 == 0) {
        g.setColour(textColour);
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(juce::MidiMessage::getMidiNoteName(note, true, true, 3), area.withTrimmedBottom(2).toNearestInt(),
                   juce::Justification::centredBottom);
    }
}

void SplitKeyboard::drawBlackNote(int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                  juce::Colour) {
    auto c = note < split ? juce::Colour(0xff26303c) : juce::Colour(0xff151515);
    if (isOver) c = c.brighter(0.15f);
    if (isDown) c = note < split ? colours::blue.darker(0.3f) : colours::orange.darker(0.3f);
    g.setColour(c);
    g.fillRect(area);
    g.setColour(c.brighter(0.25f));
    g.fillRect(area.reduced(area.getWidth() * 0.15f, 0).withTrimmedBottom(area.getHeight() * 0.1f).withHeight(3.0f));
}

// ---------------------------------------------------------------------------

Teng40Editor::Teng40Editor(Teng40Processor& p) : AudioProcessorEditor(p), proc(p), keyboard(p.keyboardState) {
    setLookAndFeel(&lnf);

    // Tone tabs (radio group bound to the "tone" choice parameter).
    for (int i = 0; i < teng::kNumTones; ++i) {
        auto* b = toneButtons.add(new juce::TextButton(juce::String(i + 1).paddedLeft('0', 2) + "\n"
                                                       + teng::kTones[(size_t)i].name));
        b->setColour(juce::TextButton::buttonColourId, colours::cream);
        b->setColour(juce::TextButton::buttonOnColourId, colours::orange.brighter(0.3f));
        b->onClick = [this, i] { toneAtt->setValueAsCompleteGesture((float)i); };
        addAndMakeVisible(b);
    }
    toneAtt = std::make_unique<juce::ParameterAttachment>(*proc.apvts.getParameter("tone"), [this](float v) {
        for (int i = 0; i < toneButtons.size(); ++i)
            toneButtons[i]->setToggleState(i == juce::roundToInt(v), juce::dontSendNotification);
    });
    toneAtt->sendInitialUpdate();

    for (int i = 0; i < teng::kNumRhythms; ++i) {
        auto* b = rhythmButtons.add(new juce::TextButton(juce::String(teng::kRhythms[(size_t)i].name).toUpperCase()));
        b->setColour(juce::TextButton::buttonColourId, colours::orange);
        b->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffc070));
        b->onClick = [this, i] { rhythmAtt->setValueAsCompleteGesture((float)i); };
        addAndMakeVisible(b);
    }
    rhythmAtt = std::make_unique<juce::ParameterAttachment>(*proc.apvts.getParameter("rhythm"), [this](float v) {
        const int idx = juce::roundToInt(v);
        for (int i = 0; i < rhythmButtons.size(); ++i)
            rhythmButtons[i]->setToggleState(i == idx, juce::dontSendNotification);
        const auto& r = teng::kRhythms[(size_t)juce::jlimit(0, teng::kNumRhythms - 1, idx)];
        ledLength = r.length();
        ledStepsPerBeat = r.stepsPerBeat;
        repaint(ledArea);
    });
    rhythmAtt->sendInitialUpdate();

    setupFader(melodyVol, "melodyVol", "MELODY");
    setupFader(bassVol, "bassVol", "BASS");
    setupFader(drumVol, "drumVol", "RHYTHM");
    setupFader(tempo, "tempo", "TEMPO");
    setupFader(lofi, "lofi", "LO-FI");
    setupFader(masterVol, "masterVol", "VOLUME");
    tempo.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 44, 16);
    tempo.slider.setNumDecimalPlacesToDisplay(0);

    const auto lit = juce::Colour(0xffffd27a);
    setupToggle(rhythmOn, "rhythmOn", "START / STOP", colours::red.brighter(0.4f));
    rhythmOn.button.setColour(juce::TextButton::buttonColourId, colours::red);
    setupToggle(synchro, "synchro", "SYNCHRO", lit);
    setupToggle(autoBass, "autoBass", "AUTO BASS", lit);
    setupToggle(hostSync, "hostSync", "HOST SYNC", lit);
    setupToggle(vibrato, "vibrato", "VIBRATO", lit);
    setupToggle(sustain, "sustain", "SUSTAIN", lit);
    setupToggle(velocity, "velocity", "VELOCITY", lit);
    setupToggle(speaker, "speaker", "SPEAKER", lit);

    fillButton.setColour(juce::TextButton::buttonColourId, colours::silver);
    fillButton.onClick = [this] { proc.requestFill(); };
    addAndMakeVisible(fillButton);

    setupStepper(transpose, transposeLabel, transposeAtt, "transpose", "TRANSPOSE");
    setupStepper(bassOctave, bassOctaveLabel, bassOctaveAtt, "bassOctave", "BASS OCT");
    setupStepper(splitNote, splitLabel, splitAtt, "splitNote", "SPLIT");
    splitNote.textFromValueFunction = [](double v) {
        return juce::MidiMessage::getMidiNoteName((int)v, true, true, 3);
    };
    splitNote.updateText();

    keyboard.setAvailableRange(24, 96);
    keyboard.setLowestVisibleKey(24);
    keyboard.setKeyWidth(18.0f);
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff9a968c));
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(0.3f));
    keyboard.setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, colours::panelHi);
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff555048));
    keyboard.setKeyPressBaseOctave(4);
    addAndMakeVisible(keyboard);

    setSize(1060, 500);
    startTimerHz(30);
}

Teng40Editor::~Teng40Editor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void Teng40Editor::setupFader(Fader& f, const juce::String& id, const juce::String& name) {
    f.slider.setSliderStyle(juce::Slider::LinearVertical);
    f.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    f.label.setText(name, juce::dontSendNotification);
    f.label.setJustificationType(juce::Justification::centred);
    f.label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    f.label.setMinimumHorizontalScale(0.7f);
    f.att = std::make_unique<SliderAttachment>(proc.apvts, id, f.slider);
    addAndMakeVisible(f.slider);
    addAndMakeVisible(f.label);
}

void Teng40Editor::setupToggle(Toggle& t, const juce::String& id, const juce::String& name, juce::Colour on) {
    t.button.setButtonText(name);
    t.button.setClickingTogglesState(true);
    t.button.setColour(juce::TextButton::buttonColourId, colours::silver.darker(0.1f));
    t.button.setColour(juce::TextButton::buttonOnColourId, on);
    t.att = std::make_unique<ButtonAttachment>(proc.apvts, id, t.button);
    addAndMakeVisible(t.button);
}

void Teng40Editor::setupStepper(juce::Slider& s, juce::Label& l, std::unique_ptr<SliderAttachment>& att,
                                const juce::String& id, const juce::String& name) {
    s.setSliderStyle(juce::Slider::IncDecButtons);
    s.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 44, 20);
    s.setColour(juce::TextButton::buttonColourId, colours::silver);
    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    l.setJustificationType(juce::Justification::centredLeft);
    att = std::make_unique<SliderAttachment>(proc.apvts, id, s);
    addAndMakeVisible(s);
    addAndMakeVisible(l);
}

void Teng40Editor::timerCallback() {
    const int step = proc.uiStep.load();
    const bool running = proc.uiRunning.load();
    if (step != ledStep || running != ledRunning) {
        ledStep = step;
        ledRunning = running;
        repaint(ledArea);
    }
    const int split = (int)proc.apvts.getRawParameterValue("splitNote")->load();
    if (split != keyboard.split) {
        keyboard.split = split;
        keyboard.repaint();
    }
}

void Teng40Editor::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();

    // body
    g.setGradientFill(juce::ColourGradient(colours::panelHi, 0, 0, colours::panel, 0, b.getHeight(), false));
    g.fillAll();

    // brushed-metal top strip
    auto strip = brandArea.toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff9ea2a7), 0, strip.getY(), juce::Colour(0xff6d7176), 0,
                                           strip.getBottom(), false));
    g.fillRect(strip);
    for (int i = 0; i < (int)strip.getHeight(); i += 2) {
        g.setColour(juce::Colours::white.withAlpha(i % 4 == 0 ? 0.04f : 0.0f));
        g.drawHorizontalLine((int)strip.getY() + i, strip.getX(), strip.getRight());
    }
    g.setColour(juce::Colour(0xff1a1a1a));
    g.setFont(juce::FontOptions(30.0f, juce::Font::bold | juce::Font::italic));
    g.drawText("TENG-40", brandArea.reduced(20, 0), juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("ELECTRONIC MINI KEYBOARD  /  RHYTHM & AUTO BASS", brandArea.reduced(20, 0).withTrimmedLeft(150),
               juce::Justification::centredLeft);

    // stripes (orange / red / brown 80s trim)
    const juce::Colour stripe[] = { colours::orange, colours::red, juce::Colour(0xff7a3a1e) };
    for (int i = 0; i < 3; ++i) {
        g.setColour(stripe[i]);
        g.fillRect(strip.getRight() - 230.0f + (float)i * 20.0f, strip.getY(), 12.0f, strip.getHeight());
    }

    auto section = [&](juce::Rectangle<int> r, const juce::String& title) {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(colours::dimText.withAlpha(0.4f));
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);
        g.setColour(colours::dimText);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(title, r.getX() + 8, r.getY() + 3, 200, 14, juce::Justification::centredLeft);
    };
    section(toneArea, "TONE SELECTOR");
    section(rhythmArea, "RHYTHM  /  BASS");
    section(mixArea, "MIXER");

    // step LEDs
    const int n = juce::jmax(1, ledLength);
    const float w = (float)ledArea.getWidth() / (float)n;
    for (int i = 0; i < n; ++i) {
        const bool beat = i % ledStepsPerBeat == 0;
        const bool on = ledRunning && i == ledStep;
        juce::Rectangle<float> led((float)ledArea.getX() + (float)i * w + 1.0f, (float)ledArea.getY(),
                                   juce::jmax(2.0f, w - 2.0f), (float)ledArea.getHeight());
        g.setColour(on ? colours::ledOn : (beat ? colours::ledOff.brighter(0.4f) : colours::ledOff));
        g.fillRoundedRectangle(led, 1.5f);
        if (on) {
            g.setColour(colours::ledOn.withAlpha(0.3f));
            g.fillRoundedRectangle(led.expanded(2.0f), 3.0f);
        }
    }

    // bass zone legend over the keyboard
    g.setColour(colours::blue);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    auto kb = keyboard.getBounds();
    g.drawText("BASS (below split)", kb.getX(), kb.getY() - 15, 200, 14, juce::Justification::centredLeft);
    g.setColour(colours::orange);
    g.drawText("MELODY", kb.getRight() - 200, kb.getY() - 15, 200, 14, juce::Justification::centredRight);
}

void Teng40Editor::resized() {
    auto r = getLocalBounds();
    brandArea = r.removeFromTop(52);
    r.reduce(12, 8);

    keyboard.setBounds(r.removeFromBottom(130));
    keyboard.setKeyWidth((float)keyboard.getWidth() / 43.0f); // 43 white keys in 24..96
    r.removeFromBottom(20);

    auto top = r;
    toneArea = top.removeFromLeft(470);
    top.removeFromLeft(10);
    rhythmArea = top.removeFromLeft(250);
    top.removeFromLeft(10);
    mixArea = top;

    // tone tabs: 2 rows x 11
    {
        auto a = toneArea.reduced(8).withTrimmedTop(14);
        auto controls = a.removeFromBottom(52);
        const int cols = 8, rows = 3;
        const int bw = a.getWidth() / cols, bh = juce::jmin(54, a.getHeight() / rows);
        for (int i = 0; i < toneButtons.size(); ++i)
            toneButtons[i]->setBounds(a.getX() + (i % cols) * bw, a.getY() + (i / cols) * bh, bw, bh);

        controls.removeFromTop(8);
        auto row = controls.removeFromTop(22);
        const int tw = row.getWidth() / 4;
        vibrato.button.setBounds(row.removeFromLeft(tw).reduced(2, 0));
        sustain.button.setBounds(row.removeFromLeft(tw).reduced(2, 0));
        velocity.button.setBounds(row.removeFromLeft(tw).reduced(2, 0));
        speaker.button.setBounds(row.removeFromLeft(tw).reduced(2, 0));
        controls.removeFromTop(4);
        auto row2 = controls.removeFromTop(22);
        transposeLabel.setBounds(row2.removeFromLeft(70));
        transpose.setBounds(row2.removeFromLeft(110));
    }

    // rhythm section
    {
        auto a = rhythmArea.reduced(8).withTrimmedTop(14);
        const int bw = a.getWidth() / 3;
        for (int i = 0; i < rhythmButtons.size(); ++i)
            rhythmButtons[i]->setBounds(a.getX() + (i % 3) * bw, a.getY() + (i / 3) * 30, bw, 30);
        a.removeFromTop(66);
        ledArea = a.removeFromTop(8);
        a.removeFromTop(8);
        auto row = a.removeFromTop(30);
        rhythmOn.button.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2, 0));
        fillButton.setBounds(row.reduced(2, 0));
        a.removeFromTop(4);
        row = a.removeFromTop(24);
        synchro.button.setBounds(row.removeFromLeft(row.getWidth() / 3).reduced(2, 0));
        autoBass.button.setBounds(row.removeFromLeft(row.getWidth() / 2).reduced(2, 0));
        hostSync.button.setBounds(row.reduced(2, 0));
        a.removeFromTop(6);
        row = a.removeFromTop(22);
        bassOctaveLabel.setBounds(row.removeFromLeft(62));
        bassOctave.setBounds(row.removeFromLeft(100));
        a.removeFromTop(4);
        row = a.removeFromTop(22);
        splitLabel.setBounds(row.removeFromLeft(62));
        splitNote.setBounds(row.removeFromLeft(100));
    }

    // mixer faders
    {
        auto a = mixArea.reduced(8).withTrimmedTop(16);
        Fader* faders[] = { &melodyVol, &bassVol, &drumVol, &tempo, &lofi, &masterVol };
        const int fw = a.getWidth() / 6;
        for (auto* f : faders) {
            auto col = a.removeFromLeft(fw);
            f->label.setBounds(col.removeFromBottom(16));
            f->slider.setBounds(col.reduced(2, 4));
        }
    }
}
