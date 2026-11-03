#include "TrackProcessor.h"
#include "TrackEditor.h"
#include "../Shared/Advisor.h"

using namespace mpm;

juce::AudioProcessorValueTreeState::ParameterLayout TrackProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    juce::StringArray roles;
    for (int i = 0; i < kNumRoles; ++i) roles.add (Advisor::roleName ((Role) i));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "role", 1 }, "Role", roles, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "analyse", 1 }, "Analyse", true));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "trim", 1 }, "Trim",
                                                        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                                        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

TrackProcessor::TrackProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MayProMixTrack", createLayout()),
      bus (SharedBus::get())
{
    ownerId = (juce::uint32) juce::Random::getSystemRandom().nextInt() | 1u;
    trimParam = apvts.getRawParameterValue ("trim");
    roleParam = apvts.getRawParameterValue ("role");
    analyseParam = apvts.getRawParameterValue ("analyse");
    slot = bus.claimSlot (ownerId);
    seenReset = bus.bus().header.resetCounter;
    if (slot >= 0)
    {
        seenTrimRequest = bus.bus().slots[slot.load()].trimRequestSeq;
        seenExcludeRequest = bus.bus().slots[slot.load()].excludeRequestSeq;
    }
    analyzer.setPhaseOffset (ownerId);
    PluginCatalog::get().retain();   // avvia la scansione in background
    timerCallback();
    startTimerHz (4);
}

TrackProcessor::~TrackProcessor()
{
    stopTimer();
    bus.releaseSlot (slot.load(), ownerId);
    PluginCatalog::get().release();
}

bool TrackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void TrackProcessor::prepareToPlay (double sampleRate, int)
{
    analyzer.prepare (sampleRate);
    gain.reset (sampleRate, 0.05);
    gain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (trimParam->load()));
}

void TrackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    lastProcessMs.store (SharedBus::now(), std::memory_order_relaxed);
    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    if (n <= 0 || numCh <= 0) return;
    for (int c = numCh; c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, n);

    // azzeramento richiesto dal Master o dalla traccia
    const auto rc = bus.bus().header.resetCounter;
    if (rc != seenReset || localReset.exchange (false))
    {
        seenReset = rc;
        analyzer.reset();
        const int sl = slot.load (std::memory_order_acquire);
        if (sl >= 0 && sl < kMaxSlots && bus.bus().slots[sl].ownerId == ownerId)
            bus.bus().slots[sl].resetEpoch = bus.bus().slots[sl].resetEpoch + 1;
    }

    // misura prima del trim: il trim viene sommato da chi legge, cosi' i cambi di trim valgono subito
    if (analyzer.process (buffer.getArrayOfReadPointers(), numCh, n))
        publish (numCh);

    gain.setTargetValue (juce::Decibels::decibelsToGain (trimParam->load()));
    if (gain.isSmoothing() || std::abs (gain.getTargetValue() - 1.0f) > 1.0e-6f)
    {
        for (int i = 0; i < n; ++i)
        {
            const float gv = gain.getNextValue();
            for (int c = 0; c < numCh; ++c) buffer.getWritePointer (c)[i] *= gv;
        }
    }
}

void TrackProcessor::processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    // plug-in in bypass: l'audio passa invariato e la traccia viene esclusa dall'analisi
    lastBypassMs.store (SharedBus::now(), std::memory_order_relaxed);
}

void TrackProcessor::publish (int numCh)
{
    const int sl = slot.load (std::memory_order_acquire);   // letto una sola volta: il timer puo' cambiarlo
    if (sl < 0 || sl >= kMaxSlots) return;
    auto& s = bus.bus().slots[sl];
    if (s.ownerId != ownerId) return;
    analyzer.getResults (results);
    s.lastSeenMs = SharedBus::now();
    s.numChannels = numCh;
    s.shortLufs = results.shortLufs;
    s.momentaryLufs = results.momentaryLufs;
    s.integratedLufs = results.integratedLufs;
    s.recentPeakDb = results.recentPeakDb;
    s.maxPeakDb = results.maxPeakDb;
    s.crestDb = results.crestDb;
    s.correlation = results.correlation;
    s.lowCorrelation = results.lowCorrelation;
    s.activeSeconds = results.activeSeconds;
    for (int b = 0; b < kNumBands; ++b)
    {
        s.bandsLong[b] = results.bandsLong[b];
        s.bandsShort[b] = results.bandsShort[b];
    }
}

void TrackProcessor::timerCallback()
{
    int sl = slot.load();
    if (sl < 0 || sl >= kMaxSlots || bus.bus().slots[sl].ownerId != ownerId)
    {
        // nessuno slot, oppure e' stato riassegnato: ne prendo un altro
        sl = bus.claimSlot (ownerId);
        slot.store (sl, std::memory_order_release);
        if (sl < 0) return;
        seenTrimRequest = bus.bus().slots[sl].trimRequestSeq;
        seenExcludeRequest = bus.bus().slots[sl].excludeRequestSeq;
    }
    auto& s = bus.bus().slots[sl];
    s.lastSeenMs = SharedBus::now();

    // richiesta di esclusione / inclusione dal Master
    const auto eseq = s.excludeRequestSeq;
    std::atomic_thread_fence (std::memory_order_acquire);
    if (eseq != seenExcludeRequest)
    {
        seenExcludeRequest = eseq;
        setAnalyse (s.excludeRequestValue == 0);
    }
    const bool bypassed = lastBypassMs.load() > lastProcessMs.load();
    s.excluded = (analyseParam->load() < 0.5f || bypassed) ? 1 : 0;

    const auto seq = s.trimRequestSeq;
    std::atomic_thread_fence (std::memory_order_acquire);
    if (seq != seenTrimRequest)
    {
        seenTrimRequest = seq;
        const float target = s.trimRequestDb;      // valore assoluto: ripetere la richiesta non fa danni
        if (std::isfinite (target) && std::abs (target) <= 24.0f)
            setTrimTarget (target);
    }
    s.trimDb = trimParam->load();
    s.role = (int) effectiveRole();
    juce::String name; juce::Colour col;
    {
        const juce::SpinLock::ScopedLockType nl (nameLock);
        name = trackName; col = trackColour;
    }
    s.colour = col.getARGB();
    std::memset (s.name, 0, sizeof (s.name));
    name.copyToUTF8 (s.name, sizeof (s.name) - 1);
}

juce::String TrackProcessor::getTrackName() const
{
    const juce::SpinLock::ScopedLockType nl (nameLock);
    return trackName;
}

Role TrackProcessor::effectiveRole() const
{
    const auto r = (Role) juce::roundToInt (roleParam->load());
    if (r != Role::Auto) return r;
    return guessRoleFromName (getTrackName());
}

void TrackProcessor::applyLevelDelta (float db)
{
    if (auto* p = apvts.getParameter ("trim"))
    {
        const float nv = juce::jlimit (-24.0f, 24.0f, trimParam->load() + db);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (nv));
        p->endChangeGesture();
    }
}

void TrackProcessor::setAnalyse (bool on)
{
    if (auto* p = apvts.getParameter ("analyse"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

void TrackProcessor::setTrimTarget (float db)
{
    if (auto* p = apvts.getParameter ("trim"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-24.0f, 24.0f, db)));
        p->endChangeGesture();
    }
}

void TrackProcessor::updateTrackProperties (const TrackProperties& props)
{
    const juce::SpinLock::ScopedLockType nl (nameLock);
    trackName = props.name;
    trackColour = props.colour;
}

void TrackProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void TrackProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TrackProcessor::createEditor() { return new TrackEditor (*this); }

#if ! MAY_TEST_HOST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TrackProcessor(); }
#endif
