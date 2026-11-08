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

    // ---- correzione integrata
    auto hz = [] (float v, int) { return v >= 1000.0f ? mpm::num (v / 1000.0f, v >= 10000.0f ? 1 : 2) + "k" : juce::String (juce::roundToInt (v)); };
    auto dB = [] (float v, int) { return mpm::num (v, 1, true); };
    auto ms = [] (float v, int) { return v < 10.0f ? mpm::num (v, 1) : juce::String (juce::roundToInt (v)); };
    auto logRange = [] (float lo, float hi) { juce::NormalisableRange<float> r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };
    auto fl = [&l] (const char* id, const char* name, juce::NormalisableRange<float> r, float def, std::function<juce::String (float, int)> fmt, const char* unit)
    {
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                                                            juce::AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (fmt)));
    };
    auto bl = [&l] (const char* id, const char* name, bool def) { l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };

    bl ("fix_on", "Fix enable", true);
    bl ("hpf_on", "HPF on", false);
    fl ("hpf_freq", "HPF freq", logRange (20.0f, 600.0f), 80.0f, hz, "Hz");
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "hpf_slope", 1 }, "HPF slope", juce::StringArray { "12 dB/oct", "24 dB/oct" }, 0));
    const float defF[3] { 250.0f, 1000.0f, 4000.0f };
    for (int k = 0; k < 3; ++k)
    {
        const auto p = "eq" + juce::String (k + 1) + "_";
        const auto nm = "EQ " + juce::String (k + 1) + " ";
        bl ((p + "on").toRawUTF8(), (nm + "on").toRawUTF8(), false);
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { p + "type", 1 }, nm + "type", juce::StringArray { "Bell", "Low shelf", "High shelf" }, 0));
        fl ((p + "freq").toRawUTF8(), (nm + "freq").toRawUTF8(), logRange (20.0f, 20000.0f), defF[k], hz, "Hz");
        fl ((p + "gain").toRawUTF8(), (nm + "gain").toRawUTF8(), juce::NormalisableRange<float> (-15.0f, 15.0f, 0.1f), 0.0f, dB, "dB");
        fl ((p + "q").toRawUTF8(), (nm + "Q").toRawUTF8(), logRange (0.3f, 8.0f), 1.0f, [] (float v, int) { return mpm::num (v, 2); }, "");
    }
    bl ("comp_on", "Comp on", false);
    fl ("comp_thr", "Comp threshold", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f, dB, "dB");
    fl ("comp_ratio", "Comp ratio", logRange (1.0f, 20.0f), 3.0f, [] (float v, int) { return mpm::num (v, 1) + ":1"; }, "");
    fl ("comp_att", "Comp attack", logRange (0.5f, 200.0f), 10.0f, ms, "ms");
    fl ("comp_rel", "Comp release", logRange (10.0f, 2000.0f), 100.0f, ms, "ms");
    fl ("comp_makeup", "Comp makeup", juce::NormalisableRange<float> (0.0f, 18.0f, 0.1f), 0.0f, dB, "dB");
    bl ("deess_on", "De-ess on", false);
    fl ("deess_freq", "De-ess freq", logRange (3000.0f, 12000.0f), 6500.0f, hz, "Hz");
    fl ("deess_thr", "De-ess threshold", juce::NormalisableRange<float> (-30.0f, 0.0f, 0.1f), -14.0f, dB, "dB");
    fl ("deess_range", "De-ess range", juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), 6.0f, dB, "dB");
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
    for (auto& id : fixParamIds())
        fixParams.add (apvts.getRawParameterValue (id));
    slot = bus.claimSlot (ownerId);
    seenReset = bus.bus().header.resetCounter;
    if (slot >= 0)
    {
        seenTrimRequest = bus.bus().slots[slot.load()].trimRequestSeq;
        seenExcludeRequest = bus.bus().slots[slot.load()].excludeRequestSeq;
        seenFixRequest = bus.bus().slots[slot.load()].fixRequestSeq;
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
    fixChain.prepare (sampleRate);
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

    // correzione integrata (prima dell'analisi: cosi' il plug-in verifica il risultato delle correzioni)
    fixChain.setSettings (currentSettings());
    fixChain.process (buffer.getArrayOfWritePointers(), numCh, n);

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

const juce::StringArray& TrackProcessor::fixParamIds()
{
    // stesso ordine di mpm::FixStateIndex
    static const juce::StringArray ids { "fix_on", "hpf_on", "hpf_freq", "hpf_slope",
        "eq1_on", "eq1_type", "eq1_freq", "eq1_gain", "eq1_q",
        "eq2_on", "eq2_type", "eq2_freq", "eq2_gain", "eq2_q",
        "eq3_on", "eq3_type", "eq3_freq", "eq3_gain", "eq3_q",
        "comp_on", "comp_thr", "comp_ratio", "comp_att", "comp_rel", "comp_makeup",
        "deess_on", "deess_freq", "deess_thr", "deess_range" };
    jassert (ids.size() == fsCount);
    return ids;
}

FixSettings TrackProcessor::currentSettings() const noexcept
{
    auto v = [this] (int i) { return fixParams[i]->load (std::memory_order_relaxed); };
    FixSettings s;
    s.enabled = v (fsEnabled) > 0.5f;
    s.hpfOn = v (fsHpfOn) > 0.5f; s.hpfFreq = v (fsHpfFreq); s.hpfSlope = v (fsHpfSlope) > 0.5f ? 24 : 12;
    for (int k = 0; k < 3; ++k)
    {
        const int b = fsEq0On + k * 5;
        s.eqOn[k] = v (b) > 0.5f; s.eqType[k] = juce::jlimit (0, 2, juce::roundToInt (v (b + 1)));
        s.eqFreq[k] = v (b + 2); s.eqGain[k] = v (b + 3); s.eqQ[k] = v (b + 4);
    }
    s.compOn = v (fsCompOn) > 0.5f; s.compThr = v (fsCompThr); s.compRatio = v (fsCompRatio);
    s.compAtt = v (fsCompAtt); s.compRel = v (fsCompRel); s.compMakeup = v (fsCompMakeup);
    s.deessOn = v (fsDeessOn) > 0.5f; s.deessFreq = v (fsDeessFreq); s.deessThr = v (fsDeessThr); s.deessRange = v (fsDeessRange);
    return s;
}

void TrackProcessor::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

void TrackProcessor::applyFix (const FixAction& f)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (! f.valid()) return;
    const auto s = currentSettings();
    setParam ("fix_on", 1.0f);

    switch (f.type)
    {
        case FixType::Hpf:
            setParam ("hpf_on", 1.0f);
            setParam ("hpf_freq", juce::jlimit (20.0f, 600.0f, f.freq));
            setParam ("hpf_slope", f.slope >= 24 ? 1.0f : 0.0f);
            break;

        case FixType::Eq:
        {
            // stessa zona di una banda gia' attiva -> la aggiorno; altrimenti la prima libera; altrimenti la meno incisiva
            int slotK = -1;
            for (int k = 0; k < 3 && slotK < 0; ++k)
                if (s.eqOn[k] && s.eqType[k] == f.shape && std::abs (std::log2 (juce::jmax (1.0f, s.eqFreq[k]) / juce::jmax (1.0f, f.freq))) < 0.6f)
                    slotK = k;
            const bool update = slotK >= 0;
            for (int k = 0; k < 3 && slotK < 0; ++k) if (! s.eqOn[k]) slotK = k;
            if (slotK < 0)
            {
                slotK = 0;
                for (int k = 1; k < 3; ++k) if (std::abs (s.eqGain[k]) < std::abs (s.eqGain[slotK])) slotK = k;
            }
            const auto p = "eq" + juce::String (slotK + 1) + "_";
            const float g = update ? s.eqGain[slotK] + f.gain : f.gain;
            setParam (p + "on", 1.0f);
            setParam (p + "type", (float) f.shape);
            setParam (p + "freq", juce::jlimit (20.0f, 20000.0f, f.freq));
            setParam (p + "gain", juce::jlimit (-15.0f, 15.0f, g));
            setParam (p + "q", juce::jlimit (0.3f, 8.0f, f.q));
            break;
        }

        case FixType::Comp:
            setParam ("comp_on", 1.0f);
            setParam ("comp_thr", juce::jlimit (-60.0f, 0.0f, f.thr));
            setParam ("comp_ratio", juce::jlimit (1.0f, 20.0f, f.ratio));
            setParam ("comp_att", juce::jlimit (0.5f, 200.0f, f.attack));
            setParam ("comp_rel", juce::jlimit (10.0f, 2000.0f, f.release));
            setParam ("comp_makeup", juce::jlimit (0.0f, 18.0f, f.makeup));
            break;

        case FixType::CompSoften:
            if (s.compOn)
            {
                setParam ("comp_thr", juce::jmin (0.0f, s.compThr + 4.0f));
                setParam ("comp_ratio", juce::jmax (1.5f, s.compRatio * 0.75f));
                setParam ("comp_makeup", juce::jmax (0.0f, s.compMakeup - 2.0f));
            }
            break;

        case FixType::DeEss:
            setParam ("deess_on", 1.0f);
            setParam ("deess_freq", juce::jlimit (3000.0f, 12000.0f, f.freq));
            setParam ("deess_thr", juce::jlimit (-30.0f, 0.0f, f.thr));
            setParam ("deess_range", juce::jlimit (0.0f, 12.0f, f.range));
            break;

        default: break;
    }
    requestReset();   // nuova analisi: cosi' la verifica "Risolto / Ancora presente" usa il suono corretto
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
        seenFixRequest = bus.bus().slots[sl].fixRequestSeq;
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
    // stato della correzione integrata e richieste del Master
    for (int i = 0; i < fsCount; ++i) s.fixState[i] = fixParams[i]->load();
    const auto fseq = s.fixRequestSeq;
    std::atomic_thread_fence (std::memory_order_acquire);
    if (fseq != seenFixRequest)
    {
        seenFixRequest = fseq;
        applyFix (FixAction::fromArray (s.fixRequest));
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
