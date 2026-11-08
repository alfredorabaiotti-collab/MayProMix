#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/Analyzer.h"
#include "../Shared/SharedBus.h"

class MasterProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    MasterProcessor();
    ~MasterProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "May Pro Mix Master"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    void resetAll();               // azzera l'analisi di master e tracce
    bool storeReference();         // memorizza lo spettro attuale come riferimento
    bool hasReference() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void publish();

    mpm::SharedBus& bus;
    mpm::Analyzer analyzer;
    mpm::AnalysisResults results;
    std::atomic<bool> pendingReset { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterProcessor)
};
