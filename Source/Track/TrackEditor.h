#pragma once
#include "TrackProcessor.h"
#include "../Shared/Theme.h"
#include "../Shared/Tutorial.h"

class TrackEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TrackEditor (TrackProcessor&);
    ~TrackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshTexts();

    TrackProcessor& proc;
    mpm::ui::LookAndFeel lnf;
    juce::Image logo;

    juce::ComboBox roleBox;
    juce::Slider trim;
    juce::TextButton resetBtn;
    mpm::ui::LangSwitch lang;
    mpm::ui::Ring loudRing;
    mpm::ui::SpectrumView spectrum;
    mpm::ui::AdviceList advice;
    juce::TextButton helpBtn { "?" };
    juce::TextButton analyseBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> analyseAttach;
    mpm::ui::Tutorial tutorial;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trimAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> roleAttach;

    juce::Rectangle<int> topArea, leftArea, specArea, adviceArea, statsArea;
    mpm::TrackInfo info;
    bool hasInfo = false, trackOk = false;
    int tick = 0;
    bool lastEnglish = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackEditor)
};
