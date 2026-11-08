#include "MasterProcessor.h"
#include "MasterEditor.h"
#include "../Shared/Advisor.h"

using namespace mpm;

juce::AudioProcessorValueTreeState::ParameterLayout MasterProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "genre", 1 }, "Genre", Advisor::genreChoices(), 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "target", 1 }, "Loudness target", Advisor::targetChoices(), 0));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "useref", 1 }, "Use reference", false));
    return l;
}

MasterProcessor::MasterProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MayProMixMaster", createLayout()),
      bus (SharedBus::get())
{
    PluginCatalog::get().retain();
    analyzer.setPhaseOffset (0x4d415354u);
    // un nuovo Master (nuovo brano) riparte senza spunte del mix precedente
    std::memset (bus.bus().header.mixDone, 0, sizeof (bus.bus().header.mixDone));
    timerCallback();
    startTimerHz (4);
}

MasterProcessor::~MasterProcessor()
{
    stopTimer();
    bus.bus().header.masterLastSeen = 0;
    PluginCatalog::get().release();
}

bool MasterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void MasterProcessor::prepareToPlay (double sampleRate, int)
{
    analyzer.prepare (sampleRate);
}

void MasterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (pendingReset.exchange (false))
        analyzer.reset();
    const int numCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    if (numCh <= 0 || buffer.getNumSamples() <= 0) return;
    if (analyzer.process (buffer.getArrayOfReadPointers(), numCh, buffer.getNumSamples()))
        publish();
}

void MasterProcessor::publish()
{
    analyzer.getResults (results);
    auto& h = bus.bus().header;
    h.masterShort = results.shortLufs;
    h.masterMomentary = results.momentaryLufs;
    h.masterIntegrated = results.integratedLufs;
    h.masterRecentPeak = results.recentPeakDb;
    h.masterMaxPeak = results.maxPeakDb;
    h.masterCrest = results.crestDb;
    h.masterCorrelation = results.correlation;
    h.masterLowCorrelation = results.lowCorrelation;
    h.masterActiveSeconds = results.activeSeconds;
    for (int b = 0; b < kNumBands; ++b)
    {
        h.masterBandsLong[b] = results.bandsLong[b];
        h.masterBandsShort[b] = results.bandsShort[b];
    }
}

void MasterProcessor::timerCallback()
{
    auto& h = bus.bus().header;
    h.masterLastSeen = SharedBus::now();
    h.genre = juce::roundToInt (apvts.getRawParameterValue ("genre")->load());
    h.loudTarget = juce::roundToInt (apvts.getRawParameterValue ("target")->load());
    h.useReference = apvts.getRawParameterValue ("useref")->load() > 0.5f ? 1 : 0;

    auto ref = apvts.state.getProperty ("reference").toString();
    juce::StringArray parts;
    parts.addTokens (ref, ";", {});
    if (parts.size() == kNumBands + 1)
    {
        for (int b = 0; b < kNumBands; ++b) h.refBands[b] = parts[b].getFloatValue();
        h.refLufs = parts[kNumBands].getFloatValue();
        h.refValid = 1;
    }
    else
    {
        h.refValid = 0;
    }
}

void MasterProcessor::resetAll()
{
    pendingReset = true;
    auto& h = bus.bus().header;
    h.resetCounter = h.resetCounter + 1;
    h.masterActiveSeconds = 0;
    h.masterIntegrated = kSilenceDb;
}

bool MasterProcessor::storeReference()
{
    const auto& h = bus.bus().header;
    if (h.masterActiveSeconds < 10.0f) return false;
    juce::StringArray parts;
    for (int b = 0; b < kNumBands; ++b) parts.add (juce::String (h.masterBandsLong[b], 2));
    parts.add (juce::String (h.masterIntegrated, 2));
    apvts.state.setProperty ("reference", parts.joinIntoString (";"), nullptr);
    if (auto* p = apvts.getParameter ("useref")) p->setValueNotifyingHost (1.0f);
    timerCallback();
    resetAll();   // da qui in poi si analizza il tuo mix
    return true;
}

bool MasterProcessor::hasReference() const
{
    return apvts.state.getProperty ("reference").toString().isNotEmpty();
}

void MasterProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MasterProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MasterProcessor::createEditor() { return new MasterEditor (*this); }

#if ! MAY_TEST_HOST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MasterProcessor(); }
#endif
