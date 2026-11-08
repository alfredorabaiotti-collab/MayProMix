#pragma once
#include "TrackProcessor.h"
#include "PresetWriter.h"
#include "../Shared/Theme.h"

// Pannello della correzione integrata di May Pro Mix Track + etichette per trascinare i preset nativi
namespace mpm::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

// Un modulo: titolo, interruttore, eventuale menu, alcune manopole con etichetta
class ModuleBox : public juce::Component
{
public:
    ModuleBox (APVTS& state, const juce::String& onId, const juce::String& comboId,
               const juce::StringArray& knobIds, std::function<float()> grSource = nullptr)
        : apvts (state), gr (std::move (grSource))
    {
        onBtn.setClickingTogglesState (true);
        addAndMakeVisible (onBtn);
        onAttach = std::make_unique<APVTS::ButtonAttachment> (apvts, onId, onBtn);

        if (comboId.isNotEmpty())
        {
            combo = std::make_unique<juce::ComboBox> ("small");
            if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (comboId)))
                combo->addItemList (p->choices, 1);
            addAndMakeVisible (*combo);
            comboAttach = std::make_unique<APVTS::ComboBoxAttachment> (apvts, comboId, *combo);
        }
        for (auto& id : knobIds)
        {
            auto* k = knobs.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox));
            k->setName ("compact");
            k->setPopupDisplayEnabled (true, true, nullptr);
            addAndMakeVisible (k);
            attaches.add (new APVTS::SliderAttachment (apvts, id, *k));
            if (auto* p = apvts.getParameter (id)) k->setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        }
    }

    void setTexts (const juce::String& t, const juce::StringArray& labels, const juce::StringArray& comboItems = {})
    {
        title = t; knobLabels = labels;
        onBtn.setButtonText ("ON");
        if (combo != nullptr && ! comboItems.isEmpty())
        {
            const int sel = combo->getSelectedItemIndex();
            combo->clear (juce::dontSendNotification);
            combo->addItemList (comboItems, 1);
            combo->setSelectedItemIndex (sel, juce::dontSendNotification);
        }
        repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 6);
        auto head = r.removeFromTop (22);
        onBtn.setBounds (head.removeFromRight (40));
        const bool comboInBody = combo != nullptr && knobs.size() <= 1;
        if (combo != nullptr && ! comboInBody)
        {
            head.removeFromRight (6);
            combo->setBounds (head.removeFromRight (juce::jlimit (60, 118, head.getWidth() - 40)));
        }
        titleArea = head;
        r.removeFromTop (4);
        if (gr) r.removeFromRight (12);
        if (comboInBody)
        {
            auto side = r.removeFromRight (r.getWidth() / 2);
            combo->setBounds (side.withSizeKeepingCentre (juce::jmin (side.getWidth() - 4, 84), 24).translated (0, -7));
        }
        const int n = knobs.size();
        if (n == 0) return;
        const int w = r.getWidth() / n;
        for (int i = 0; i < n; ++i)
            knobs[i]->setBounds (r.removeFromLeft (w).withTrimmedBottom (14).withSizeKeepingCentre (juce::jmin (w, 56), juce::jmin (r.getHeight() - 14, 56)));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = onBtn.getToggleState();
        g.setColour (C::well);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (on ? C::teal2 : juce::Colour (0xff232c37));
        g.drawRoundedRectangle (r, 8.0f, 1.0f);
        g.setColour (on ? C::text : C::dim);
        g.setFont (font (11.0f, true));
        g.drawFittedText (title.toUpperCase(), titleArea.isEmpty() ? getLocalBounds().reduced (10, 6).removeFromTop (22) : titleArea.withTrimmedLeft (2),
                          juce::Justification::centredLeft, 1, 0.8f);

        g.setColour (C::dim);
        g.setFont (font (10.0f));
        for (int i = 0; i < knobs.size() && i < knobLabels.size(); ++i)
        {
            auto kb = knobs[i]->getBounds();
            g.drawText (knobLabels[i], kb.getX() - 10, kb.getBottom(), kb.getWidth() + 20, 13, juce::Justification::centred);
        }

        if (gr)
        {
            // misura della riduzione di guadagno (0-12 dB)
            auto m = getLocalBounds().reduced (8, 6).withTrimmedTop (26).removeFromRight (6).toFloat();
            g.setColour (juce::Colour (0xff1a2028));
            g.fillRoundedRectangle (m, 3.0f);
            const float v = juce::jlimit (0.0f, 12.0f, gr());
            g.setColour (C::amber);
            g.fillRoundedRectangle (m.withHeight (m.getHeight() * v / 12.0f), 3.0f);
        }
    }

    bool hasMeter() const noexcept { return gr != nullptr; }

private:
    APVTS& apvts;
    std::function<float()> gr;
    juce::TextButton onBtn;
    std::unique_ptr<juce::ComboBox> combo;
    juce::OwnedArray<juce::Slider> knobs;
    std::unique_ptr<APVTS::ButtonAttachment> onAttach;
    std::unique_ptr<APVTS::ComboBoxAttachment> comboAttach;
    juce::OwnedArray<APVTS::SliderAttachment> attaches;
    juce::String title;
    juce::StringArray knobLabels;
    juce::Rectangle<int> titleArea;
};

// Etichetta da trascinare sulla traccia di Studio Pro: crea il preset al momento del trascinamento
class PresetChip : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<bool (const juce::File&)> writer;   // scrive il preset; false = nulla da esportare
    std::function<juce::String()> fileName;
    juce::String label;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (isEnabled() ? C::teal.withAlpha (over ? 0.28f : 0.16f) : C::panel2);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (isEnabled() ? C::teal : C::line);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        // icona: freccia verso il basso
        auto ic = r.removeFromLeft (r.getHeight()).reduced (7);
        juce::Path p;
        p.startNewSubPath (ic.getCentreX(), ic.getY()); p.lineTo (ic.getCentreX(), ic.getBottom() - 2);
        p.startNewSubPath (ic.getX() + 2, ic.getCentreY() + 1); p.lineTo (ic.getCentreX(), ic.getBottom() - 2); p.lineTo (ic.getRight() - 2, ic.getCentreY() + 1);
        g.setColour (isEnabled() ? C::teal : C::dim);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (isEnabled() ? C::text : C::dim);
        g.setFont (font (12.0f, true));
        g.drawText (label, r.withTrimmedRight (10), juce::Justification::centredLeft, true);
    }

    void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { over = false; repaint(); }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! writer) return;
        const auto f = presets::dragFolder().getChildFile (fileName ? fileName() : juce::String ("May Pro Mix.preset"));
        if (! writer (f)) return;
        dragging = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this,
                                                                   [safe = juce::Component::SafePointer<PresetChip> (this)]
                                                                   { if (safe != nullptr) safe->dragging = false; });
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        dragging = false;
        if (e.getDistanceFromDragStart() >= 6 || ! writer) return;
        // clic semplice: crea il file e lo mostra nella cartella, da cui si puo' trascinare
        const auto f = presets::dragFolder().getChildFile (fileName ? fileName() : juce::String ("May Pro Mix.preset"));
        if (writer (f)) f.revealToUser();
    }

private:
    bool over = false, dragging = false;
};

class FixPanel : public juce::Component, private juce::Timer
{
public:
    explicit FixPanel (TrackProcessor& p)
        : proc (p),
          hpf (p.apvts, "hpf_on", "hpf_slope", { "hpf_freq" }),
          eq1 (p.apvts, "eq1_on", "eq1_type", { "eq1_freq", "eq1_gain", "eq1_q" }),
          eq2 (p.apvts, "eq2_on", "eq2_type", { "eq2_freq", "eq2_gain", "eq2_q" }),
          eq3 (p.apvts, "eq3_on", "eq3_type", { "eq3_freq", "eq3_gain", "eq3_q" }),
          comp (p.apvts, "comp_on", {}, { "comp_thr", "comp_ratio", "comp_att", "comp_rel", "comp_makeup" }, [&p] { return p.getCompGr(); }),
          deess (p.apvts, "deess_on", {}, { "deess_freq", "deess_thr", "deess_range" }, [&p] { return p.getDeessGr(); })
    {
        for (auto* m : { &hpf, &eq1, &eq2, &eq3, &comp, &deess }) addAndMakeVisible (m);
        masterBtn.setClickingTogglesState (true);
        addAndMakeVisible (masterBtn);
        masterAttach = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "fix_on", masterBtn);

        eqChip.label = "Pro EQ3";
        eqChip.fileName = [this] { return presets::safeName (proc.getTrackName()) + " - May Pro Mix EQ.preset"; };
        eqChip.writer = [this] (const juce::File& f) { return presets::writeProEq (proc.currentSettings(), f, presets::safeName (proc.getTrackName()) + " May Pro Mix"); };
        compChip.label = "Compressor";
        compChip.fileName = [this] { return presets::safeName (proc.getTrackName()) + " - May Pro Mix Comp.preset"; };
        compChip.writer = [this] (const juce::File& f)
        {
            const auto s = proc.currentSettings();
            return s.enabled && s.compOn && presets::writeCompressor (s, f, presets::safeName (proc.getTrackName()) + " May Pro Mix");
        };
        addAndMakeVisible (eqChip);
        addAndMakeVisible (compChip);
        refreshTexts();
        startTimerHz (12);
    }

    void refreshTexts()
    {
        masterBtn.setButtonText (tr ("Correzione ON", "Fix ON"));
        hpf.setTexts (tr ("Passa-alto", "High-pass"), { "Hz" }, { "12 dB", "24 dB" });
        const juce::StringArray types { tr ("Campana", "Bell"), tr ("Shelf bassi", "Low shelf"), tr ("Shelf alti", "High shelf") };
        eq1.setTexts ("EQ 1", { "Hz", "dB", "Q" }, types);
        eq2.setTexts ("EQ 2", { "Hz", "dB", "Q" }, types);
        eq3.setTexts ("EQ 3", { "Hz", "dB", "Q" }, types);
        comp.setTexts (tr ("Compressore", "Compressor"), { tr ("Soglia", "Thresh"), "Ratio", tr ("Attacco", "Attack"), tr ("Rilascio", "Release"), "Gain" });
        deess.setTexts ("De-esser", { "Hz", tr ("Soglia", "Thresh"), tr ("Max dB", "Range") });
        const auto tip = tr ("Trascina sulla traccia di Studio Pro per avere gli stessi valori nel plug-in nativo (poi spegni la correzione integrata). Clic: mostra il file.",
                             "Drag onto the Studio Pro track to get the same values in the native plug-in (then switch off the built-in fix). Click: show the file.");
        eqChip.setTooltip (tip);
        compChip.setTooltip (tip);
        repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12, 8);
        auto head = r.removeFromTop (26);
        masterBtn.setBounds (head.removeFromRight (120));
        head.removeFromRight (10);
        compChip.setBounds (head.removeFromRight (124));
        head.removeFromRight (6);
        eqChip.setBounds (head.removeFromRight (104));
        head.removeFromRight (8);
        chipsLabel = head.removeFromRight (150);
        r.removeFromTop (8);
        auto row1 = r.removeFromTop (r.getHeight() / 2 - 3);
        r.removeFromTop (6);
        auto row2 = r;
        const int gap = 8;
        const int w1 = (row1.getWidth() - 3 * gap);
        hpf.setBounds (row1.removeFromLeft (w1 * 22 / 100)); row1.removeFromLeft (gap);
        const int we = (row1.getWidth() - 2 * gap) / 3;
        eq1.setBounds (row1.removeFromLeft (we)); row1.removeFromLeft (gap);
        eq2.setBounds (row1.removeFromLeft (we)); row1.removeFromLeft (gap);
        eq3.setBounds (row1);
        comp.setBounds (row2.removeFromLeft (row2.getWidth() * 62 / 100)); row2.removeFromLeft (gap);
        deess.setBounds (row2);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds(), tr ("Correzione integrata", "Built-in fix"));
        g.setColour (C::dim);
        g.setFont (font (11.0f));
        g.drawText (tr ("Preset nativi da trascinare:", "Native presets to drag:"), chipsLabel, juce::Justification::centredRight, true);
    }

private:
    void timerCallback() override
    {
        comp.repaint();
        deess.repaint();
        const auto s = proc.currentSettings();
        bool anyEq = s.enabled && s.hpfOn;
        for (int k = 0; k < 3; ++k) anyEq = anyEq || (s.enabled && s.eqOn[k] && std::abs (s.eqGain[k]) > 0.05f);
        eqChip.setEnabled (anyEq);
        compChip.setEnabled (s.enabled && s.compOn);
    }

    TrackProcessor& proc;
    ModuleBox hpf, eq1, eq2, eq3, comp, deess;
    juce::TextButton masterBtn;
    std::unique_ptr<APVTS::ButtonAttachment> masterAttach;
    PresetChip eqChip, compChip;
    juce::Rectangle<int> chipsLabel;
};
} // namespace mpm::ui
