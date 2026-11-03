#include "TrackEditor.h"
#include "BinaryData.h"

using namespace mpm;
using namespace mpm::ui;

TrackEditor::TrackEditor (TrackProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
    lastEnglish = SharedBus::get().english();

    roleBox.addItemList (Advisor::roleChoices(), 1);
    addAndMakeVisible (roleBox);
    roleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "role", roleBox);

    trim.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    trim.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    trim.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (trim);
    trimAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "trim", trim);

    resetBtn.onClick = [this] { proc.requestReset(); };
    analyseBtn.setClickingTogglesState (true);
    analyseAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "analyse", analyseBtn);
    analyseBtn.onStateChange = [this] { analyseBtn.setButtonText (analyseBtn.getToggleState() ? tr ("Analizza", "Analyse") : tr ("Esclusa", "Excluded")); };
    addAndMakeVisible (analyseBtn);
    addAndMakeVisible (resetBtn);

    lang.onChange = [this] { refreshTexts(); };
    addAndMakeVisible (lang);
    addAndMakeVisible (loudRing);
    addAndMakeVisible (spectrum);
    addAndMakeVisible (advice);
    advice.onApplyLevel = [this] (float target) { proc.setTrimTarget (target); };
    spectrum.onGoToAdvice = [this] (int band) { advice.focusBand (band); };
    // solo cambio di stato: le schede vengono ricostruite dal timer (mai dentro il click)
    advice.onDoneChanged = [this] (const Advice& a, bool on)
    {
        if (proc.getSlot() < 0) return;   // senza slot non scriviamo nulla (eviterebbe di toccare i consigli del mix)
        SharedBus::get().setDone (proc.getSlot(), a.key, a.title, on);
        tick = 0;
    };
    helpBtn.onClick = [this] { tutorial.start(); };
    addAndMakeVisible (helpBtn);
    addChildComponent (tutorial);

    refreshTexts();
    setResizable (true, true);
    setResizeLimits (1000, 580, 1600, 1100);
    setSize (1040, 660);
    if (! Tutorial::alreadySeen())
        tutorial.start();
    startTimerHz (15);
}

TrackEditor::~TrackEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TrackEditor::refreshTexts()
{
    lastEnglish = SharedBus::get().english();
    const int sel = roleBox.getSelectedItemIndex();
    roleBox.clear (juce::dontSendNotification);
    roleBox.addItemList (Advisor::roleChoices(), 1);
    roleBox.setSelectedItemIndex (sel, juce::dontSendNotification);
    resetBtn.setButtonText (tr ("Azzera analisi", "Reset analysis"));
    analyseBtn.setButtonText (analyseBtn.getToggleState() ? tr ("Analizza", "Analyse") : tr ("Esclusa", "Excluded"));
    analyseBtn.setTooltip (tr ("Spegni quando la traccia e' in mute", "Turn off when the track is muted"));
    trim.setTooltip (tr ("Trim di uscita: usalo per bilanciare il mix", "Output trim: use it to balance the mix"));
    tutorial.update();
    tick = 0;
    repaint();
    timerCallback();
}

void TrackEditor::resized()
{
    auto r = getLocalBounds().reduced (10);
    topArea = r.removeFromTop (58);
    r.removeFromTop (10);
    leftArea = r.removeFromLeft (260);
    r.removeFromLeft (10);
    specArea = r.removeFromTop (juce::jmax (200, r.getHeight() * 2 / 5));
    r.removeFromTop (10);
    adviceArea = r;

    auto t = topArea.reduced (10, 13);
    t.removeFromLeft (92 + 12 + 130);
    t.removeFromLeft (180);   // nome traccia
    roleBox.setBounds (t.removeFromLeft (218).withTrimmedLeft (8));
    t.removeFromLeft (8);
    analyseBtn.setBounds (t.removeFromLeft (96));
    lang.setBounds (t.removeFromRight (88));
    t.removeFromRight (8);
    helpBtn.setBounds (t.removeFromRight (32));
    t.removeFromRight (8);
    resetBtn.setBounds (t.removeFromRight (124));

    auto l = leftArea.reduced (14);
    l.removeFromTop (20);
    trim.setBounds (l.removeFromTop (150).withSizeKeepingCentre (150, 150));
    l.removeFromTop (8);
    loudRing.setBounds (l.removeFromTop (150).withSizeKeepingCentre (150, 150));
    l.removeFromTop (12);
    statsArea = l.removeFromTop (110);

    spectrum.setBounds (specArea.reduced (12, 10).withTrimmedTop (4));
    advice.setBounds (adviceArea.reduced (12).withTrimmedTop (34));
    tutorial.setBounds (getLocalBounds());
}

void TrackEditor::paint (juce::Graphics& g)
{
    g.fillAll (C::bg);
    drawPanel (g, topArea);
    drawPanel (g, leftArea);
    drawPanel (g, specArea, tr ("Spettro della traccia", "Track spectrum"));
    drawPanel (g, adviceArea);

    auto t = topArea.reduced (10, 6);
    drawLogo (g, logo, t.removeFromLeft (92));
    t.removeFromLeft (12);
    drawBrand (g, t.removeFromLeft (130), "TRACK");

    auto namePill = t.removeFromLeft (180).reduced (0, 7);
    g.setColour (C::panel2);
    g.fillRoundedRectangle (namePill.toFloat(), namePill.getHeight() * 0.5f);
    auto colour = hasInfo && info.colour.getAlpha() > 0 ? info.colour : C::teal;
    g.setColour (colour);
    g.fillEllipse (namePill.getX() + 12.0f, namePill.getCentreY() - 5.0f, 10.0f, 10.0f);
    g.setColour (C::text);
    g.setFont (font (13.0f, true));
    auto nm = proc.getTrackName();
    if (nm.isEmpty()) nm = tr ("Traccia senza nome", "Unnamed track");
    g.drawText (nm, namePill.withTrimmedLeft (30).withTrimmedRight (10), juce::Justification::centredLeft, true);

    g.setColour (C::dim);
    g.setFont (font (11.0f, true));
    g.drawText ("TRIM", leftArea.reduced (14).removeFromTop (16), juce::Justification::centred);

    // statistiche
    auto s = statsArea;
    const int w = (s.getWidth() - 8) / 2;
    auto row1 = s.removeFromTop (50), row2 = s.withTrimmedTop (8);
    const bool ok = hasInfo && info.activeSeconds > 0.5f;
    drawStat (g, row1.removeFromLeft (w), ok ? num (info.maxPeakDb) + " dB" : "-", tr ("Picco", "Peak"));
    drawStat (g, row1.withTrimmedLeft (8), ok ? num (info.crestDb) + " dB" : "-", tr ("Cresta", "Crest"));
    drawStat (g, row2.removeFromLeft (w), ok && ! std::isnan (info.relLevel) ? num (info.relLevel, 1, true) + " LU" : "-", tr ("Rel. mix", "Rel. mix"));
    drawStat (g, row2.withTrimmedLeft (8), ok && info.analysed() ? juce::String (juce::roundToInt (info.fullness)) : "-",
              tr ("Pienezza: ", "Fullness: ") + (ok && info.analysed() ? Advisor::fullnessLabel (info.fullness) : juce::String ("-")));

    auto ah = adviceArea.reduced (14, 10).removeFromTop (24);
    g.setColour (C::text);
    g.setFont (font (15.0f, true));
    const auto advTitle = tr ("Consigli", "Advice");
    g.drawText (advTitle, ah, juce::Justification::centredLeft);
    if (trackOk)
    {
        const auto okTxt = tr ("Traccia corretta", "Track OK");
        const int tw = juce::GlyphArrangement::getStringWidthInt (font (12.0f, true), okTxt);
        auto pill = juce::Rectangle<int> (ah.getX() + juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), advTitle) + 12,
                                          ah.getCentreY() - 11, tw + 38, 22);
        g.setColour (C::teal);
        g.fillRoundedRectangle (pill.toFloat(), 11.0f);
        drawCheck (g, pill.removeFromLeft (26).toFloat().reduced (7, 5), juce::Colour (0xff0f1a1c), 2.0f);
        g.setColour (juce::Colour (0xff0f1a1c));
        g.setFont (font (12.0f, true));
        g.drawText (okTxt, pill, juce::Justification::centredLeft);
    }
    g.setColour (C::dim);
    g.setFont (font (11.5f));
    const auto& gp = genreProfile (SharedBus::get().bus().header.genre);
    juce::String right = tr (gp.nameIt, gp.nameEn) + "  |  " + Advisor::roleName (proc.effectiveRole());
    if (! SharedBus::isFresh (SharedBus::get().bus().header.masterLastSeen))
        right = tr ("Master non trovato  |  ", "Master not found  |  ") + right;
    g.drawText (right, ah, juce::Justification::centredRight);
}

void TrackEditor::timerCallback()
{
    if (SharedBus::get().english() != lastEnglish)
    {
        refreshTexts();
        lang.repaint();
        return;
    }

    const auto snap = Advisor::capture();
    const int idx = snap.indexOfSlot (proc.getSlot());
    hasInfo = idx >= 0;
    if (hasInfo) info = snap.tracks.getReference (idx);

    if (hasInfo)
    {
        const bool any = info.activeSeconds > 0.5f;
        loudRing.set (any ? info.integratedLufs : -60.0f, -50.0f, -6.0f,
                      any ? num (info.integratedLufs) : "--", "LUFS " + tr ("integrata", "integrated"));
        spectrum.set (info.bandsLong, info.bandsShort, profileFor (info.role).bands, 4.0f,
                      info.name, tr ("Profilo: ", "Profile: ") + Advisor::roleName (info.role), any);

        if (tick++ % 8 == 0)
        {
            auto list = Advisor::forTrack (snap, idx);
            Advisor::applyDoneMarks (list, proc.getSlot(), info.analysed());
            trackOk = info.analysed();
            for (auto& a : list) if (a.status != 3 && a.severity >= 1) trackOk = false;
            spectrum.setMarkers (list);
            advice.setAdvice (list, 2, true);
        }
    }
    repaint (leftArea);
    repaint (topArea);
    repaint (adviceArea.withHeight (44));
}
