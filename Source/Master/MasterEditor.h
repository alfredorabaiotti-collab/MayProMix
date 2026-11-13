#pragma once
#include "MasterProcessor.h"
#include "../Shared/Theme.h"
#include "../Shared/Tutorial.h"
#include "../Shared/Donate.h"

// Elenco delle tracce collegate con meter, livello relativo, pienezza e stato
class TrackRows : public juce::Component
{
public:
    std::function<void (int)> onSelect;   // slot selezionato, -1 = MIX
    std::function<void (int, bool)> onToggleExclude;   // slot, nuovo stato "esclusa"
    int selectedSlot = -1;

    void setData (const mpm::Snapshot& s, const juce::Array<int>& worst)
    {
        snap = s; worstSev = worst;
        setSize (getWidth(), (snap.tracks.size() + 1) * rowH);
        repaint();
    }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int row = e.y / rowH;
        const int slot = row == 0 ? -1 : (row - 1 < snap.tracks.size() ? snap.tracks.getReference (row - 1).slot : -2);
        if (slot >= 0)
        {
            juce::Rectangle<int> c[7];
            layoutColumns ({ 0, row * rowH, getWidth(), rowH }, c);
            if (c[6].expanded (8, 0).contains (e.getPosition()))
            {
                if (onToggleExclude) onToggleExclude (slot, ! snap.tracks.getReference (row - 1).excluded);
                return;
            }
        }
        if (slot >= -1)
        {
            selectedSlot = slot;
            if (onSelect) onSelect (slot);
            repaint();
        }
    }

    static constexpr int rowH = 44;
    static void layoutColumns (juce::Rectangle<int> r, juce::Rectangle<int>* cols);

private:
    mpm::Snapshot snap;
    juce::Array<int> worstSev;
};

// Tracce ordinate per gravita' dei problemi (barra di sinistra)
class IssueList : public juce::Component
{
public:
    struct Item { int slot; juce::String name; juce::Colour colour; int important = 0, suggested = 0, score = 0; };
    std::function<void (int)> onSelect;
    int selectedSlot = -1;

    void setItems (juce::Array<Item> list)
    {
        std::stable_sort (list.begin(), list.end(), [] (const Item& a, const Item& b) { return a.score > b.score; });
        items = list;
        int okCount = 0;
        for (auto& it : items) if (it.score == 0) ++okCount;
        // le tracce corrette sono "chip" su piu' righe: stima generosa dell'altezza
        const int chipRows = okCount > 0 ? (okCount + 1) / 2 : 0;
        const int needed = 54 + (okCount < items.size() ? 20 : 0) + (okCount > 0 ? 24 + chipRows * 30 : 0)
                           + juce::jmax (1, items.size() - okCount) * rowH + 8;
        setSize (getWidth(), needed);   // dentro un Viewport: si scorre se le tracce sono tante
        repaint();
    }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto& rs : rowSlots)
            if (e.y >= rs.top && e.y < rs.bottom && e.x >= rs.left && e.x < rs.right && rs.slot >= 0)
            {
                if (onSelect) onSelect (rs.slot);
                return;
            }
    }

    static constexpr int rowH = 28;

private:
    struct RowSlot { int top, bottom, slot, left = 0, right = 100000; };
    juce::Array<Item> items;
    juce::Array<RowSlot> rowSlots;   // posizione delle righe disegnate (per il clic)
};

class MasterEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MasterEditor (MasterProcessor&);
    ~MasterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshTexts();
    void balanceAll();

    MasterProcessor& proc;
    mpm::ui::LookAndFeel lnf;
    juce::Image logo;

    juce::ComboBox genreBox, targetBox;
    juce::TextButton refBtn, storeBtn, resetBtn, helpBtn { "?" }, balanceBtn;
    mpm::ui::LangSwitch lang;
    mpm::ui::Ring loudRing, fullRing;
    mpm::ui::SpectrumView spectrum;
    juce::Viewport rowsView;
    TrackRows rows;
    IssueList issues;
    juce::Viewport issuesView;
    mpm::ui::AdviceList advice;
    mpm::ui::Tutorial tutorial;
    may::Welcome welcome { "MayProMix", "May Pro Mix", mpm::ui::welcomeEnglish(), mpm::ui::welcomeSetEnglish(), mpm::ui::welcomeStyle() };
    may::DonateButton donateBtn { mpm::ui::welcomeEnglish(), mpm::ui::welcomeStyle() };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> genreAttach, targetAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> refAttach;

    juce::Rectangle<int> topArea, leftArea, specArea, listArea, adviceArea, statsArea;
    mpm::Snapshot snap;
    juce::String toast;
    int toastTicks = 0, tick = 0;
    bool lastEnglish = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterEditor)
};
