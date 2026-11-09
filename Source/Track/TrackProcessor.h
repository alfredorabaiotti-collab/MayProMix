#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Shared/Analyzer.h"
#include "../Shared/SharedBus.h"
#include "../Shared/Advisor.h"
#include "../Shared/FixModel.h"

class TrackProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    TrackProcessor();
    ~TrackProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "May Pro Mix Track"; }
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
    void updateTrackProperties (const TrackProperties&) override;

    juce::AudioProcessorValueTreeState apvts;
    int getSlot() const noexcept { return slot.load(); }
    juce::String getTrackName() const;
    mpm::Role effectiveRole() const;
    void requestReset() noexcept { localReset = true; }
    void applyLevelDelta (float db);
    void setTrimTarget (float db);
    void setAnalyse (bool on);
    void applyFixes (const juce::Array<mpm::FixAction>& fixes);   // aggiunge le correzioni ai valori dei preset (thread dei messaggi)
    void setSettings (const mpm::FixSettings& s);
    mpm::FixSettings currentSettings() const noexcept;
    static const juce::StringArray& fixParamIds();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void publish (int numCh);

    mpm::SharedBus& bus;
    mpm::Analyzer analyzer;
    mpm::AnalysisResults results;
    juce::uint32 ownerId;
    std::atomic<int> slot { -1 };
    juce::uint32 seenReset = 0, seenTrimRequest = 0, seenExcludeRequest = 0, seenFixRequest = 0;
    juce::Array<std::atomic<float>*> fixParams;
    void setParam (const juce::String& id, float value);
    std::atomic<juce::int64> lastProcessMs { 0 }, lastBypassMs { 0 };
    std::atomic<float>* analyseParam = nullptr;
    std::atomic<bool> localReset { false };

    juce::SmoothedValue<float> gain;
    std::atomic<float>* trimParam = nullptr;
    std::atomic<float>* roleParam = nullptr;

    mutable juce::SpinLock nameLock;
    juce::String trackName;
    juce::Colour trackColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackProcessor)
};
