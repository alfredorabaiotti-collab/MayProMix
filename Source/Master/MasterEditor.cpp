#include "MasterEditor.h"
#include "BinaryData.h"

using namespace mpm;
using namespace mpm::ui;

//==============================================================================
void TrackRows::layoutColumns (juce::Rectangle<int> r, juce::Rectangle<int>* c)
{
    r = r.withTrimmedLeft (14).withTrimmedRight (14);
    c[0] = r.removeFromLeft (16);  r.removeFromLeft (10);
    c[6] = r.removeFromRight (18); r.removeFromRight (8);
    c[5] = r.removeFromRight (64); r.removeFromRight (12);
    c[4] = r.removeFromRight (62); r.removeFromRight (8);
    c[3] = r.removeFromRight (62); r.removeFromRight (12);
    c[1] = r.removeFromLeft (r.getWidth() * 45 / 100); r.removeFromLeft (10);
    c[2] = r;
}

void TrackRows::paint (juce::Graphics& g)
{
    const auto& m = snap.mix;
    const float tgt = targetLufs (m.loudTarget, m.genre);

    auto drawMeter = [&g] (juce::Rectangle<int> r, float value, float marker)
    {
        auto bar = r.withSizeKeepingCentre (r.getWidth(), 10).toFloat();
        g.setColour (juce::Colour (0xff1a2028));
        g.fillRoundedRectangle (bar, 5.0f);
        const float f = juce::jlimit (0.0f, 1.0f, (value + 60.0f) / 60.0f);
        if (f > 0.0f)
        {
            auto fill = bar.withWidth (bar.getWidth() * f);
            juce::ColourGradient gr (C::teal2, bar.getX(), 0, C::red, bar.getRight(), 0, false);
            gr.addColour (0.7, C::teal);
            gr.addColour (0.9, C::amber);
            g.setGradientFill (gr);
            g.fillRoundedRectangle (fill, 5.0f);
        }
        if (! std::isnan (marker) && marker > -60.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (bar.getX() + bar.getWidth() * juce::jlimit (0.0f, 1.0f, (marker + 60.0f) / 60.0f) - 1.0f, bar.getY() - 3, 2.0f, bar.getHeight() + 6);
        }
    };
    auto drawFull = [&g] (juce::Rectangle<int> r, float v)
    {
        auto bar = r.withSizeKeepingCentre (r.getWidth(), 6).toFloat();
        g.setColour (juce::Colour (0xff1a2028));
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (C::teal);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, v / 100.0f)), 3.0f);
    };

    for (int row = 0; row <= snap.tracks.size(); ++row)
    {
        auto r = juce::Rectangle<int> (0, row * rowH, getWidth(), rowH);
        const bool isMix = row == 0;
        const int slot = isMix ? -1 : snap.tracks.getReference (row - 1).slot;
        if (slot == selectedSlot)
        {
            g.setColour (juce::Colour (0xff2f3b48));
            g.fillRect (r);
            g.setColour (C::teal);
            g.fillRect (r.withWidth (3));
        }
        g.setColour (juce::Colour (0xff222a34));
        g.drawHorizontalLine (r.getY(), 0.0f, (float) getWidth());

        juce::Rectangle<int> c[7];
        layoutColumns (r, c);

        if (isMix)
        {
            g.setGradientFill (juce::ColourGradient (C::teal, 0, (float) r.getY(), C::teal2, 0, (float) r.getBottom(), false));
            g.fillRoundedRectangle (c[0].withSizeKeepingCentre (14, 26).toFloat(), 4.0f);
            g.setColour (C::text); g.setFont (font (13.0f, true));
            g.drawText ("MIX", c[1].withY (r.getY() + 5).withHeight (18), juce::Justification::centredLeft);
            g.setColour (C::dim); g.setFont (font (11.0f));
            g.drawText (m.masterPresent ? tr ("Canale Main", "Main channel") : tr ("Master non trovato", "Master not found"),
                        c[1].withY (r.getY() + 23).withHeight (15), juce::Justification::centredLeft);
            drawMeter (c[2], m.shortLufs, m.integratedLufs);
            g.setColour (C::text); g.setFont (font (12.5f));
            g.drawText (m.analysed() ? num (m.integratedLufs) : "-", c[3], juce::Justification::centredLeft);
            g.setColour (C::dim); g.setFont (font (11.5f));
            g.drawText (m.loudTarget == targetPreMaster ? "-" : num (tgt, 0), c[4], juce::Justification::centredLeft);
            if (m.analysed()) drawFull (c[5], m.fullness);
            continue;
        }

        const auto& t = snap.tracks.getReference (row - 1);
        const int sevRow = row - 1 < worstSev.size() ? worstSev[row - 1] : -1;
        const bool okTrack = ! t.excluded && t.analysed() && sevRow == 0;
        if (okTrack && slot != selectedSlot)
        {
            g.setGradientFill (juce::ColourGradient (C::teal.withAlpha (0.10f), 0, 0, C::teal.withAlpha (0.0f), (float) getWidth() * 0.6f, 0, false));
            g.fillRect (r.withTrimmedTop (1));
        }
        g.saveState();
        if (t.excluded) g.setOpacity (0.4f);
        g.setColour (t.colour.getAlpha() > 0 ? t.colour : C::teal2);
        g.fillRoundedRectangle (c[0].withSizeKeepingCentre (14, 26).toFloat(), 4.0f);
        g.setColour (C::text); g.setFont (font (13.0f, true));
        g.drawText (t.name, c[1].withY (r.getY() + 5).withHeight (18), juce::Justification::centredLeft, true);
        g.setColour (C::dim); g.setFont (font (11.0f));
        auto sub = c[1].withY (r.getY() + 23).withHeight (15);
        if (okTrack)
        {
            drawCheck (g, sub.removeFromLeft (12).toFloat().reduced (0, 1), C::teal, 1.8f);
            sub.removeFromLeft (3);
            g.setColour (C::teal); g.setFont (font (11.0f, true));
            const auto okTxt = tr ("Corretta", "OK");
            g.drawText (okTxt, sub, juce::Justification::centredLeft);
            sub.removeFromLeft (juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), okTxt) + 6);
            g.setColour (C::dim); g.setFont (font (11.0f));
        }
        g.drawText (t.excluded ? tr ("Esclusa (mute o bypass)", "Excluded (mute or bypass)") : Advisor::roleName (t.role),
                    sub, juce::Justification::centredLeft, true);
        drawMeter (c[2], t.shortLufs, t.analysed() ? t.integratedLufs : mpm::kNaN);
        g.setColour (C::text); g.setFont (font (12.5f));
        g.drawText (std::isnan (t.relLevel) ? "-" : num (t.relLevel, 1, true), c[3], juce::Justification::centredLeft);
        g.setColour (C::dim); g.setFont (font (11.5f));
        g.drawText (std::isnan (t.targetRel) ? "-" : num (t.targetRel, 1, true), c[4], juce::Justification::centredLeft);
        if (t.analysed()) drawFull (c[5], t.fullness);
        const int sev = row - 1 < worstSev.size() ? worstSev[row - 1] : -1;
        g.restoreState();
        // interruttore analisi: pallino pieno = analizzata (colore = gravita'), anello vuoto = esclusa
        auto dot = c[6].withSizeKeepingCentre (12, 12).toFloat();
        if (t.excluded)
        {
            g.setColour (C::dim);
            g.drawEllipse (dot.reduced (0.5f), 1.5f);
        }
        else if (okTrack)
        {
            drawOkBadge (g, c[6].withSizeKeepingCentre (18, 18).toFloat());
        }
        else
        {
            g.setColour (sev >= 0 ? C::severity (sev) : C::line);
            g.fillEllipse (dot.reduced (1.0f));
        }
    }
}

//==============================================================================
void IssueList::paint (juce::Graphics& g)
{
    rowSlots.clearQuick();
    auto area = getLocalBounds();

    // contatore delle tracce corrette
    int okCount = 0;
    for (auto& it : items) if (it.score == 0) ++okCount;
    {
        auto box = area.removeFromTop (46);
        g.setColour (C::well);
        g.fillRoundedRectangle (box.toFloat(), 8.0f);
        auto in = box.reduced (10, 7);
        auto top = in.removeFromTop (18);
        g.setColour (C::dim);
        g.setFont (font (10.5f, true));
        g.drawText (tr ("TRACCE CORRETTE", "TRACKS OK"), top, juce::Justification::centredLeft);
        g.setColour (C::teal);
        g.setFont (font (15.0f, true));
        g.drawText (juce::String (okCount) + " / " + juce::String (items.size()), top, juce::Justification::centredRight);
        auto bar = in.withSizeKeepingCentre (in.getWidth(), 6).withY (in.getBottom() - 8).toFloat();
        g.setColour (juce::Colour (0xff1a2028));
        g.fillRoundedRectangle (bar, 3.0f);
        if (items.size() > 0 && okCount > 0)
        {
            g.setColour (C::teal);
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * (float) okCount / (float) items.size()), 3.0f);
        }
    }
    area.removeFromTop (8);

    if (items.isEmpty())
    {
        g.setColour (C::dim);
        g.setFont (font (12.0f));
        g.drawText (tr ("Nessuna traccia analizzata", "No tracks analysed yet"), area.removeFromTop (rowH), juce::Justification::centredLeft);
        return;
    }

    auto section = [&] (const juce::String& title, juce::Colour col)
    {
        auto r = area.removeFromTop (20);
        g.setColour (col);
        g.setFont (font (10.0f, true));
        g.drawText (title.toUpperCase(), r.withTrimmedLeft (6), juce::Justification::bottomLeft);
        rowSlots.add ({ r.getY(), r.getBottom(), -2 });
    };

    auto drawItem = [&] (const Item& it, bool ok)
    {
        if (area.getHeight() < rowH) return;
        auto r = area.removeFromTop (rowH);
        rowSlots.add ({ r.getY(), r.getBottom(), it.slot });
        if (it.slot == selectedSlot || ok)
        {
            g.setColour (it.slot == selectedSlot ? juce::Colour (0xff2f3b48) : C::teal.withAlpha (0.07f));
            g.fillRoundedRectangle (r.toFloat().reduced (0, 1), 6.0f);
        }
        auto rr = r.reduced (6, 0);
        g.setColour (it.colour.getAlpha() > 0 ? it.colour : C::teal2);
        g.fillRoundedRectangle (rr.removeFromLeft (4).withSizeKeepingCentre (4, 16).toFloat(), 2.0f);
        rr.removeFromLeft (8);

        if (ok)
        {
            drawOkBadge (g, rr.removeFromRight (22).withSizeKeepingCentre (18, 18).toFloat());
        }
        else
        {
            auto badge = [&g, &rr] (int n, juce::Colour c)
            {
                auto b = rr.removeFromRight (26).withSizeKeepingCentre (22, 18);
                rr.removeFromRight (4);
                if (n <= 0) return;
                g.setColour (c);
                g.fillRoundedRectangle (b.toFloat(), 9.0f);
                g.setColour (juce::Colour (0xff0f1a1c));
                g.setFont (font (11.0f, true));
                g.drawText (juce::String (n), b, juce::Justification::centred);
            };
            badge (it.suggested, C::amber);
            badge (it.important, C::red);
        }
        g.setColour (C::text);
        g.setFont (font (12.5f, ! ok));
        g.drawText (it.name, rr, juce::Justification::centredLeft, true);
    };

    // prima le tracce corrette, ben visibili come etichette verdi
    if (okCount > 0)
    {
        section (tr ("Corrette", "OK"), C::teal);
        int x = 0, y = area.getY() + 2;
        const int chipH = 24, gap = 6;
        g.setFont (font (12.0f, true));
        for (auto& it : items)
        {
            if (it.score != 0) continue;
            const int w = juce::jmin (area.getWidth(), juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), it.name) + 38);
            if (x > 0 && x + w > area.getWidth()) { x = 0; y += chipH + gap; }
            auto chip = juce::Rectangle<int> (area.getX() + x, y, w, chipH);
            g.setColour (it.slot == selectedSlot ? C::teal : C::teal.withAlpha (0.16f));
            g.fillRoundedRectangle (chip.toFloat(), chipH * 0.5f);
            g.setColour (C::teal);
            g.drawRoundedRectangle (chip.toFloat().reduced (0.5f), chipH * 0.5f, 1.0f);
            auto inner = chip.reduced (6, 0);
            drawOkBadge (g, inner.removeFromLeft (16).withSizeKeepingCentre (16, 16).toFloat());
            inner.removeFromLeft (6);
            g.setColour (it.slot == selectedSlot ? juce::Colour (0xff0f1a1c) : C::text);
            g.drawText (it.name, inner, juce::Justification::centredLeft, true);
            rowSlots.add ({ chip.getY(), chip.getBottom(), it.slot, chip.getX(), chip.getRight() });
            x += w + gap;
        }
        area.removeFromTop (y + chipH + 10 - area.getY());
    }

    if (okCount < items.size())
    {
        section (tr ("Da sistemare", "To fix"), C::dim);
        for (auto& it : items) if (it.score > 0) drawItem (it, false);
    }
}

//==============================================================================
MasterEditor::MasterEditor (MasterProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
    lastEnglish = SharedBus::get().english();

    genreBox.addItemList (Advisor::genreChoices(), 1);
    targetBox.addItemList (Advisor::targetChoices(), 1);
    addAndMakeVisible (genreBox);
    addAndMakeVisible (targetBox);
    genreAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "genre", genreBox);
    targetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "target", targetBox);

    refBtn.setClickingTogglesState (true);
    refAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "useref", refBtn);
    storeBtn.onClick = [this]
    {
        toast = proc.storeReference() ? tr ("Riferimento memorizzato: ora fai suonare il tuo mix", "Reference stored: now play your mix")
                                      : tr ("Fai suonare il brano di riferimento almeno 10 secondi", "Play the reference song for at least 10 seconds");
        toastTicks = 60;
    };
    resetBtn.onClick = [this]
    {
        proc.resetAll();
        toast = tr ("Analisi azzerata: fai suonare il brano", "Analysis reset: play the song");
        toastTicks = 45;
    };
    helpBtn.onClick = [this] { tutorial.start(); };
    lang.onChange = [this] { refreshTexts(); };
    for (auto* c : std::initializer_list<juce::Component*> { &refBtn, &storeBtn, &resetBtn, &helpBtn, &lang, &loudRing, &fullRing, &spectrum, &rowsView, &advice })
        addAndMakeVisible (c);

    rowsView.setViewedComponent (&rows, false);
    rowsView.setScrollBarsShown (true, false);
    rowsView.setScrollBarThickness (8);
    rows.onSelect = [this] (int) { tick = 0; timerCallback(); };
    rows.onToggleExclude = [this] (int slot, bool exclude)
    {
        if (slot < 0 || slot >= kMaxSlots) return;
        auto& s = SharedBus::get().bus().slots[slot];
        s.excludeRequestValue = exclude ? 1 : 0;
        std::atomic_thread_fence (std::memory_order_release);
        s.excludeRequestSeq = s.excludeRequestSeq + 1;
        tick = 0;
    };
    issues.onSelect = [this] (int slot)
    {
        rows.selectedSlot = slot;
        tick = 0;
        timerCallback();
    };
    advice.onDoneChanged = [this] (const Advice& a, bool on)
    {
        SharedBus::get().setDone (rows.selectedSlot, a.key, a.title, on);
        tick = 0;
    };
    issuesView.setViewedComponent (&issues, false);
    issuesView.setScrollBarsShown (true, false);
    issuesView.setScrollBarThickness (6);
    addAndMakeVisible (issuesView);
    advice.onApplyLevel = [this] (float target)
    {
        // il Master chiede alla traccia di portare il Trim a un valore assoluto
        const int slot = rows.selectedSlot;
        if (slot < 0 || slot >= kMaxSlots || ! std::isfinite (target)) return;
        auto& s = SharedBus::get().bus().slots[slot];
        s.trimRequestDb = juce::jlimit (-24.0f, 24.0f, target);
        std::atomic_thread_fence (std::memory_order_release);
        s.trimRequestSeq = s.trimRequestSeq + 1;
        tick = 0;
    };
    spectrum.onGoToAdvice = [this] (int band)
    {
        rows.selectedSlot = -1;
        tick = 0;
        timerCallback();
        advice.focusBand (band);
    };

    addChildComponent (tutorial);
    refreshTexts();
    setResizable (true, true);
    setResizeLimits (1100, 700, 1800, 1200);
    setSize (1200, 780);
    if (! Tutorial::alreadySeen())
        tutorial.start();
    startTimerHz (15);
}

MasterEditor::~MasterEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MasterEditor::refreshTexts()
{
    lastEnglish = SharedBus::get().english();
    auto refill = [] (juce::ComboBox& box, const juce::StringArray& items)
    {
        const int sel = box.getSelectedItemIndex();
        box.clear (juce::dontSendNotification);
        box.addItemList (items, 1);
        box.setSelectedItemIndex (sel, juce::dontSendNotification);
    };
    refill (genreBox, Advisor::genreChoices());
    refill (targetBox, Advisor::targetChoices());
    refBtn.setButtonText (tr ("Riferimento", "Reference"));
    storeBtn.setButtonText (tr ("Memorizza riferimento", "Store reference"));
    resetBtn.setButtonText (tr ("Azzera analisi", "Reset analysis"));
    tutorial.update();
    tick = 0;
    repaint();
}

void MasterEditor::resized()
{
    auto r = getLocalBounds().reduced (10);
    topArea = r.removeFromTop (58);
    r.removeFromTop (10);
    leftArea = r.removeFromLeft (270);
    r.removeFromLeft (10);
    adviceArea = r.removeFromRight (juce::jmax (360, getWidth() / 3 - 20));
    r.removeFromRight (10);
    specArea = r.removeFromTop (250);
    r.removeFromTop (10);
    listArea = r;

    auto t = topArea.reduced (10, 13);
    t.removeFromLeft (92 + 12 + 130);
    genreBox.setBounds (t.removeFromLeft (190));
    t.removeFromLeft (8);
    targetBox.setBounds (t.removeFromLeft (230));
    t.removeFromLeft (8);
    refBtn.setBounds (t.removeFromLeft (100));
    t.removeFromLeft (6);
    storeBtn.setBounds (t.removeFromLeft (164));
    lang.setBounds (t.removeFromRight (88));
    t.removeFromRight (8);
    helpBtn.setBounds (t.removeFromRight (32));
    t.removeFromRight (8);
    resetBtn.setBounds (t.removeFromRight (124));

    auto l = leftArea.reduced (14);
    l.removeFromTop (20);
    loudRing.setBounds (l.removeFromTop (150).withSizeKeepingCentre (150, 150));
    l.removeFromTop (20);
    statsArea = l.removeFromTop (110);
    l.removeFromTop (12);
    fullRing.setBounds (l.removeFromTop (92).removeFromLeft (92));
    l.removeFromTop (14);
    issuesView.setBounds (l);
    issues.setSize (l.getWidth() - 8, juce::jmax (issues.getHeight(), 10));

    spectrum.setBounds (specArea.reduced (12, 10).withTrimmedTop (4));
    rowsView.setBounds (listArea.withTrimmedTop (34).withTrimmedBottom (6));
    rows.setSize (rowsView.getWidth() - 8, rows.getHeight());
    advice.setBounds (adviceArea.reduced (12).withTrimmedTop (34));
    tutorial.setBounds (getLocalBounds());
}

void MasterEditor::paint (juce::Graphics& g)
{
    g.fillAll (C::bg);
    drawPanel (g, topArea);
    drawPanel (g, leftArea, "Loudness");
    drawPanel (g, specArea, tr ("Bilanciamento tonale del mix", "Mix tonal balance"));
    drawPanel (g, listArea);
    drawPanel (g, adviceArea);

    auto t = topArea.reduced (10, 6);
    drawLogo (g, logo, t.removeFromLeft (92));
    t.removeFromLeft (12);
    drawBrand (g, t.removeFromLeft (130), "MASTER");

    const auto& m = snap.mix;
    const bool ok = m.masterPresent && m.activeSeconds > 0.5f;
    g.setColour (C::dim);
    g.setFont (font (11.5f));
    const float tgt = targetLufs (m.loudTarget, m.genre);
    g.drawText (m.loudTarget == targetPreMaster ? tr ("picchi obiettivo -6 dBFS", "target peaks -6 dBFS")
                                                : tr ("obiettivo ", "target ") + num (tgt, 0) + " LUFS",
                loudRing.getBounds().translated (0, 84).withHeight (16), juce::Justification::centred);

    auto s = statsArea;
    const int w = (s.getWidth() - 8) / 2;
    auto row1 = s.removeFromTop (50), row2 = s.withTrimmedTop (8);
    drawStat (g, row1.removeFromLeft (w), ok ? num (m.shortLufs) : "-", tr ("Breve termine", "Short term"));
    drawStat (g, row1.withTrimmedLeft (8), ok ? num (m.maxPeakDb) + " dB" : "-", tr ("Picco", "Peak"));
    drawStat (g, row2.removeFromLeft (w), ok ? num (m.crestDb) + " dB" : "-", tr ("Cresta", "Crest"));
    drawStat (g, row2.withTrimmedLeft (8), ok ? num (m.correlation, 2) : "-", tr ("Correlazione", "Correlation"));

    {
        auto fr = fullRing.getBounds();
        auto txt = juce::Rectangle<int> (fr.getRight() + 10, fr.getY(), leftArea.getRight() - 14 - fr.getRight() - 10, fr.getHeight());
        g.setColour (C::dim);
        g.setFont (font (11.0f, true));
        g.drawText (tr ("PIENEZZA DEL MIX", "MIX FULLNESS"), txt.removeFromTop (fr.getHeight() / 2), juce::Justification::bottomLeft);
        g.setColour (C::text);
        g.setFont (font (12.0f));
        g.drawFittedText (tr ("Copertura delle frequenze e densita' rispetto al genere", "Frequency coverage and density for the genre"),
                          txt.withTrimmedTop (4), juce::Justification::topLeft, 3);
    }

    // intestazione lista
    juce::Rectangle<int> c[7];
    TrackRows::layoutColumns (listArea.withTrimmedTop (12).withHeight (20), c);
    g.setColour (C::dim);
    g.setFont (font (10.0f, true));
    g.drawText (tr ("TRACCIA", "TRACK"), c[1], juce::Justification::centredLeft);
    g.drawText (tr ("LIVELLO (BREVE TERMINE)", "LEVEL (SHORT TERM)"), c[2], juce::Justification::centredLeft);
    g.drawText (tr ("REL. MIX", "REL. MIX"), c[3], juce::Justification::centredLeft);
    g.drawText (tr ("OBIETTIVO", "TARGET"), c[4], juce::Justification::centredLeft);
    g.drawText (tr ("PIENEZZA", "FULLNESS"), c[5], juce::Justification::centredLeft);
    g.drawText (tr ("ON", "ON"), c[6].expanded (6, 0), juce::Justification::centred);

    // intestazione consigli
    auto ah = adviceArea.reduced (14, 10).removeFromTop (24);
    juce::String head = tr ("Consigli per il mix", "Mix advice"), sub;
    const int idx = snap.indexOfSlot (rows.selectedSlot);
    const auto& gp = genreProfile (m.genre);
    if (idx >= 0) { head = snap.tracks.getReference (idx).name; sub = Advisor::roleName (snap.tracks.getReference (idx).role); }
    else sub = tr (gp.nameIt, gp.nameEn);
    g.setColour (C::text);
    g.setFont (font (15.0f, true));
    g.drawText (head, ah, juce::Justification::centredLeft, true);
    g.setColour (C::dim);
    g.setFont (font (11.5f));
    g.drawText (sub, ah, juce::Justification::centredRight);

    if (toastTicks > 0)
    {
        auto tr2 = specArea.withSizeKeepingCentre (juce::jmin (440, specArea.getWidth() - 40), 36);
        g.setColour (C::teal);
        g.fillRoundedRectangle (tr2.toFloat(), 18.0f);
        g.setColour (juce::Colour (0xff0f1a1c));
        g.setFont (font (13.0f, true));
        g.drawText (toast, tr2, juce::Justification::centred);
    }
}

void MasterEditor::timerCallback()
{
    if (SharedBus::get().english() != lastEnglish)
    {
        refreshTexts();
        lang.repaint();
    }

    snap = Advisor::capture();
    const auto& m = snap.mix;
    const bool ok = m.masterPresent && m.activeSeconds > 0.5f;

    loudRing.set (ok ? m.integratedLufs : -60.0f, -30.0f, -4.0f, ok ? num (m.integratedLufs) : "--",
                  "LUFS " + tr ("integrata", "integrated"),
                  m.loudTarget == targetPreMaster ? mpm::kNaN : targetLufs (m.loudTarget, m.genre));
    fullRing.set (m.analysed() ? m.fullness : 0.0f, 0.0f, 100.0f,
                  m.analysed() ? juce::String (juce::roundToInt (m.fullness)) : "--",
                  m.analysed() ? Advisor::fullnessLabel (m.fullness) : tr ("in analisi", "analysing"));

    const auto& gp = genreProfile (m.genre);
    spectrum.set (m.bandsLong, m.bandsShort, m.targetCurve(), 3.0f, "Mix",
                  m.refValid ? tr ("Riferimento", "Reference") : tr (gp.nameIt, gp.nameEn), ok);

    if (toastTicks > 0 && --toastTicks == 0) repaint (specArea);

    // consigli e stato delle righe: due volte al secondo
    if (tick++ % 8 == 0)
    {
        if (rows.selectedSlot >= 0 && snap.indexOfSlot (rows.selectedSlot) < 0)
            rows.selectedSlot = -1;

        juce::Array<int> worst;
        juce::Array<IssueList::Item> issueItems;
        for (int i = 0; i < snap.tracks.size(); ++i)
        {
            const auto& t = snap.tracks.getReference (i);
            int w = 0;
            IssueList::Item it { t.slot, t.name, t.colour };
            if (t.analysed())
            {
                auto list = Advisor::forTrack (snap, i);
                Advisor::applyDoneMarks (list, t.slot, true);
                for (auto& a : list)
                {
                    if (a.status == 3) continue;
                    w = juce::jmax (w, a.severity);
                    if (a.severity >= 2) ++it.important;
                    else if (a.severity == 1) ++it.suggested;
                }
                it.score = it.important * 3 + it.suggested;
                issueItems.add (it);
            }
            worst.add (t.analysed() ? w : -1);
        }
        rows.setData (snap, worst);
        issues.selectedSlot = rows.selectedSlot;
        issues.setItems (issueItems);

        const int idx = snap.indexOfSlot (rows.selectedSlot);
        auto mixAdvice = Advisor::forMix (snap);
        Advisor::applyDoneMarks (mixAdvice, -1, m.analysed());
        spectrum.setMarkers (mixAdvice);
        if (idx >= 0)
        {
            auto list = Advisor::forTrack (snap, idx);
            Advisor::applyDoneMarks (list, rows.selectedSlot, snap.tracks.getReference (idx).analysed());
            advice.setAdvice (list, 1, true);
        }
        else
        {
            advice.setAdvice (mixAdvice, 1, false);
        }
        repaint (adviceArea.withHeight (44));
    }
    else
    {
        rows.repaint();
    }
    repaint (leftArea);
}
