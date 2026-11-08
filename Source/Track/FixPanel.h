#pragma once
#include "TrackProcessor.h"
#include "../Shared/PresetWriter.h"
#include "../Shared/Theme.h"

// Pannello dei valori dei preset nativi (Pro EQ3 e Compressor) + pulsanti da trascinare nella catena insert
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

class FixPanel : public juce::Component, private juce::Timer
{
public:
    explicit FixPanel (TrackProcessor& p)
        : proc (p),
          hpf (p.apvts, "hpf_on", "hpf_slope", { "hpf_freq" }),
          eq1 (p.apvts, "eq1_on", "eq1_type", { "eq1_freq", "eq1_gain", "eq1_q" }),
          eq2 (p.apvts, "eq2_on", "eq2_type", { "eq2_freq", "eq2_gain", "eq2_q" }),
          eq3 (p.apvts, "eq3_on", "eq3_type", { "eq3_freq", "eq3_gain", "eq3_q" }),
          comp (p.apvts, "comp_on", {}, { "comp_thr", "comp_ratio", "comp_att", "comp_rel", "comp_makeup" })
    {
        for (auto* m : { &hpf, &eq1, &eq2, &eq3, &comp }) addAndMakeVisible (m);
        eqChip.makeFile = [this] { return presets::writeForTrack (proc.currentSettings(), 0, proc.getTrackName()); };
        compChip.makeFile = [this] { return presets::writeForTrack (proc.currentSettings(), 1, proc.getTrackName()); };
        eqChip.onDragged = compChip.onDragged = [this] { proc.requestReset(); };
        addAndMakeVisible (eqChip);
        addAndMakeVisible (compChip);
        refreshTexts();
        startTimerHz (12);
    }

    void refreshTexts()
    {
        eqChip.label = tr ("Trascina Pro EQ3", "Drag Pro EQ3");
        compChip.label = tr ("Trascina Compressor", "Drag Compressor");
        hpf.setTexts (tr ("Passa-alto", "High-pass"), { "Hz" }, { "12 dB", "24 dB" });
        const juce::StringArray types { tr ("Campana", "Bell"), tr ("Shelf bassi", "Low shelf"), tr ("Shelf alti", "High shelf") };
        eq1.setTexts ("EQ 1", { "Hz", "dB", "Q" }, types);
        eq2.setTexts ("EQ 2", { "Hz", "dB", "Q" }, types);
        eq3.setTexts ("EQ 3", { "Hz", "dB", "Q" }, types);
        comp.setTexts (tr ("Compressore", "Compressor"), { tr ("Soglia", "Thresh"), "Ratio", tr ("Attacco", "Attack"), tr ("Rilascio", "Release"), "Gain" });
        const auto tip = tr ("Trascina nella catena insert della traccia in Studio Pro, sopra May Pro Mix Track: il plug-in nativo arriva con questi valori. Se c'e' gia', rilascialo sopra di lui per aggiornarlo. Clic: mostra il file.",
                             "Drag into the track insert chain in Studio Pro, above May Pro Mix Track: the native plug-in arrives with these values. If it is already there, drop onto it to update it. Click: show the file.");
        eqChip.setTooltip (tip);
        compChip.setTooltip (tip);
        repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12, 8);
        auto head = r.removeFromTop (26);
        compChip.setBounds (head.removeFromRight (176));
        head.removeFromRight (8);
        eqChip.setBounds (head.removeFromRight (160));
        head.removeFromRight (8);
        chipsLabel = head.removeFromRight (260);
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
        comp.setBounds (row2);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds(), tr ("Preset per i plug-in nativi", "Native plug-in presets"));
        g.setColour (C::dim);
        g.setFont (font (11.0f));
        g.drawText (tr ("Trascina nella catena insert, sopra May Pro Mix:", "Drag into the insert chain, above May Pro Mix:"), chipsLabel, juce::Justification::centredRight, true);
    }

private:
    void timerCallback() override
    {
        const auto s = proc.currentSettings();
        eqChip.setEnabled (s.hasEq());
        compChip.setEnabled (s.compOn);
    }

    TrackProcessor& proc;
    ModuleBox hpf, eq1, eq2, eq3, comp;
    DragPresetButton eqChip, compChip;
    juce::Rectangle<int> chipsLabel;
};
} // namespace mpm::ui
