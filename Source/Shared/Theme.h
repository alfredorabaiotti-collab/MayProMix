#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "FixModel.h"

// Aspetto grafico di May Pro Mix: pannelli ardesia, accento verde acqua
namespace mpm::ui
{
namespace C
{
    const juce::Colour bg      { 0xff1c232c };
    const juce::Colour panel   { 0xff252e39 };
    const juce::Colour panel2  { 0xff2d3743 };
    const juce::Colour well    { 0xff1f2730 };
    const juce::Colour line    { 0xff36414f };
    const juce::Colour dark    { 0xff141920 };
    const juce::Colour text    { 0xffdfe7ee };
    const juce::Colour dim     { 0xff8e9bab };
    const juce::Colour teal    { 0xff5fd1bf };
    const juce::Colour teal2   { 0xff2f8577 };
    const juce::Colour amber   { 0xfff2b84b };
    const juce::Colour red     { 0xffef6b5e };

    inline juce::Colour severity (int s) { return s >= 2 ? red : (s == 1 ? amber : teal); }
}

inline juce::Font font (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

inline void drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title = {})
{
    auto rf = r.toFloat();
    g.setColour (C::panel);
    g.fillRoundedRectangle (rf, 10.0f);
    g.setColour (juce::Colour (0xff1a2029));
    g.drawRoundedRectangle (rf.reduced (0.5f), 10.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour (C::dim);
        g.setFont (font (11.0f, true));
        g.drawText (title.toUpperCase(), r.reduced (14, 10).removeFromTop (14), juce::Justification::topLeft);
    }
}

//==============================================================================
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, C::panel2);
        setColour (juce::ComboBox::textColourId, C::text);
        setColour (juce::ComboBox::outlineColourId, C::line);
        setColour (juce::ComboBox::arrowColourId, C::text);
        setColour (juce::PopupMenu::backgroundColourId, C::bg);
        setColour (juce::PopupMenu::textColourId, C::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, C::teal2);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::Label::textColourId, C::text);
        setColour (juce::TextButton::buttonColourId, C::panel2);
        setColour (juce::TextButton::buttonOnColourId, C::teal);
        setColour (juce::TextButton::textColourOffId, C::text);
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff0f1a1c));
        setColour (juce::ScrollBar::thumbColourId, C::line);
        setColour (juce::Slider::textBoxTextColourId, C::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ToggleButton::textColourId, C::text);
        setColour (juce::ToggleButton::tickColourId, C::teal);
        setColour (juce::ToggleButton::tickDisabledColourId, C::dim);
    }

    juce::Font getComboBoxFont (juce::ComboBox& b) override { return font (b.getName() == "small" ? 11.5f : 13.0f, true); }
    juce::Font getPopupMenuFont() override                { return font (14.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (12.5f, true); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (C::panel2);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (box.hasKeyboardFocus (false) ? C::teal2 : C::line);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        juce::Path p;
        const float cx = (float) w - 16.0f, cy = (float) h * 0.5f;
        p.startNewSubPath (cx - 4, cy - 2); p.lineTo (cx, cy + 2); p.lineTo (cx + 4, cy - 2);
        g.setColour (C::text);
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (12, 1, box.getWidth() - 34, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        auto fill = on ? C::teal : C::panel2;
        if (down) fill = fill.darker (0.15f); else if (over && ! on) fill = fill.brighter (0.05f);
        g.setColour (fill);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (on ? C::teal : (over ? C::teal2 : C::line));
        g.drawRoundedRectangle (r, 8.0f, 1.0f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::TwoValueVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, minPos, maxPos, style, s);
            return;
        }
        const float cx = x + w * 0.5f;
        g.setColour (juce::Colour (0xff1a2028));
        g.fillRoundedRectangle (cx - 3.0f, (float) y, 6.0f, (float) h, 3.0f);
        g.setColour (C::teal.withAlpha (0.55f));
        g.fillRoundedRectangle (cx - 3.0f, maxPos, 6.0f, minPos - maxPos, 3.0f);
        for (float p : { minPos, maxPos })
        {
            g.setColour (C::text);
            g.fillRoundedRectangle (cx - 8.0f, p - 4.0f, 16.0f, 8.0f, 4.0f);
            g.setColour (C::teal);
            g.drawRoundedRectangle (cx - 8.0f, p - 4.0f, 16.0f, 8.0f, 4.0f, 1.2f);
        }
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        const bool compact = s.getName() == "compact";
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (compact ? 2.0f : 8.0f);
        const float size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const float stroke = compact ? juce::jmax (3.0f, size * 0.08f) : 8.0f;
        const float rad = size * 0.5f - stroke * 0.75f;
        const auto c = r.getCentre();

        g.setColour (juce::Colour (0xff1b222b));
        g.fillEllipse (r.reduced (compact ? stroke * 1.6f : 16.0f));
        g.setColour (juce::Colour (0xff11161c));
        g.drawEllipse (r.reduced (compact ? stroke * 1.6f : 16.0f), compact ? 1.2f : 2.0f);

        juce::Path bgArc, valArc;
        bgArc.addCentredArc (c.x, c.y, rad, rad, 0, a0, a1, true);
        g.setColour (juce::Colour (0xff1a2028));
        g.strokePath (bgArc, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // bipolare (es. guadagno -15..+15): l'arco parte dal centro; altrimenti dall'inizio
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? a0 + (a1 - a0) * (float) ((0.0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) : a0;
        const float av = a0 + pos * (a1 - a0);
        valArc.addCentredArc (c.x, c.y, rad, rad, 0, juce::jmin (from, av), juce::jmax (from, av) + 0.001f, true);
        const bool active = s.isEnabled() && (s.getParentComponent() == nullptr || s.getParentComponent()->isEnabled());
        g.setColour (active ? C::teal : C::dim);
        g.strokePath (valArc, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const auto v = (float) s.getValue();
        g.setColour (C::text);
        if (compact)
        {
            g.setFont (font (juce::jlimit (9.0f, 13.0f, size * 0.22f), true));
            g.drawText (s.getTextFromValue (v), r.expanded (6, 0), juce::Justification::centred, false);
            return;
        }
        g.setFont (font (size * 0.17f, true));
        g.drawText (num (v, 1, true), r.withTrimmedBottom (size * 0.08f), juce::Justification::centred);
        g.setColour (C::dim);
        g.setFont (font (size * 0.085f));
        g.drawText ("dB", r.withTrimmedTop (size * 0.32f), juce::Justification::centred);
    }
};

//==============================================================================
// Anello di misura (loudness, pienezza)
class Ring : public juce::Component
{
public:
    void set (float value, float mn, float mx, const juce::String& big, const juce::String& small, float targetValue = mpm::kNaN)
    {
        v = value; lo = mn; hi = mx; label = big; sub = small; target = targetValue;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const auto c = r.getCentre();
        const float rad = size * 0.5f - 8.0f;
        const float a0 = juce::MathConstants<float>::pi * -0.75f, a1 = juce::MathConstants<float>::pi * 0.75f;

        g.setColour (juce::Colour (0xff1b222b));
        g.fillEllipse (r.reduced (size * 0.14f));
        g.setColour (juce::Colour (0xff11161c));
        g.drawEllipse (r.reduced (size * 0.14f), 2.0f);

        juce::Path bgArc;
        bgArc.addCentredArc (c.x, c.y, rad, rad, 0, a0, a1, true);
        g.setColour (juce::Colour (0xff1a2028));
        g.strokePath (bgArc, juce::PathStrokeType (size * 0.06f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float f = juce::jlimit (0.0f, 1.0f, (v - lo) / (hi - lo));
        if (f > 0.002f)
        {
            juce::Path a;
            a.addCentredArc (c.x, c.y, rad, rad, 0, a0, a0 + f * (a1 - a0), true);
            g.setColour (C::teal);
            g.strokePath (a, juce::PathStrokeType (size * 0.06f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        if (! std::isnan (target))
        {
            const float ft = juce::jlimit (0.0f, 1.0f, (target - lo) / (hi - lo));
            const float at = a0 + ft * (a1 - a0) - juce::MathConstants<float>::halfPi;
            g.setColour (juce::Colours::white);
            g.fillEllipse (juce::Rectangle<float> (8, 8).withCentre ({ c.x + rad * std::cos (at), c.y + rad * std::sin (at) }));
        }
        g.setColour (C::text);
        g.setFont (font (size * 0.16f, true));
        g.drawText (label, r.withTrimmedBottom (size * 0.1f), juce::Justification::centred);
        g.setColour (C::dim);
        g.setFont (font (size * 0.075f));
        g.drawText (sub, r.withTrimmedTop (size * 0.3f), juce::Justification::centred);
    }

private:
    float v = 0, lo = 0, hi = 1, target = mpm::kNaN;
    juce::String label, sub;
};

//==============================================================================
// Spettro a 8 bande con curva obiettivo e tolleranza
class SpectrumView : public juce::Component
{
public:
    struct Marker { int band = 0; int severity = 1; juce::String title, detail; };
    std::function<void (int band)> onGoToAdvice;

    SpectrumView()
    {
        scale.setSliderStyle (juce::Slider::TwoValueVertical);
        scale.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        scale.setRange (-60.0, 12.0, 1.0);
        scale.setMinAndMaxValues (-36.0, 3.0, juce::dontSendNotification);
        scale.setTooltip (tr ("Scala del grafico: trascina i due cursori", "Graph scale: drag the two handles"));
        scale.onValueChange = [this]
        {
            if (scale.getMaxValue() - scale.getMinValue() < 12.0)
            {
                const double lo = juce::jlimit (-60.0, 0.0, scale.getMaxValue() - 12.0);
                scale.setMinAndMaxValues (lo, lo + 12.0, juce::dontSendNotification);
            }
            repaint();
        };
        addAndMakeVisible (scale);
        scale.addMouseListener (this, false);
    }

    void resized() override
    {
        scale.setBounds (getWidth() - 20, 22, 20, getHeight() - 40);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (e.eventComponent == &scale || scale.getBounds().contains (e.getPosition()))
            scale.setMinAndMaxValues (-36.0, 3.0);
    }

    void setMarkers (const juce::Array<Advice>& advice)
    {
        juce::Array<Marker> m;
        for (auto& a : advice)
            if (a.band >= 0 && a.band < kNumBands)
            {
                bool dup = false;
                for (auto& x : m) if (x.band == a.band) dup = true;
                if (! dup) m.add ({ a.band, a.severity, a.title, a.detail });
            }
        markers = m;
        if (openMarker >= markers.size()) openMarker = -1;
        repaint();
    }

    void set (const float* longBands, const float* shortBands, const float* targetCurve, float tolerance,
              const juce::String& name, const juce::String& targetName, bool valid)
    {
        std::copy (longBands, longBands + kNumBands, lng);
        std::copy (shortBands, shortBands + kNumBands, sht);
        std::copy (targetCurve, targetCurve + kNumBands, tgt);
        tol = tolerance; nameA = name; nameB = targetName; hasData = valid;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto plot = area.withTrimmedLeft (28).withTrimmedBottom (18).withTrimmedTop (22).withTrimmedRight (30);
        const float top = (float) scale.getMaxValue();
        const float bottom = juce::jmin ((float) scale.getMinValue(), top - 1.0f);   // mai divisione per zero
        auto X = [&plot] (float f) { return plot.getX() + plot.getWidth() * std::log10 (f / 20.0f) / 3.0f; };
        auto Y = [&plot, top, bottom] (float d) { return plot.getY() + plot.getHeight() * (top - juce::jlimit (bottom, top, d)) / (top - bottom); };

        g.setFont (font (10.0f));
        const float step = (top - bottom) > 42.0f ? 12.0f : 6.0f;
        juce::Array<float> lines;
        for (float d = std::floor (top / step) * step; d >= bottom; d -= step) lines.add (d);
        for (float d : lines)
        {
            g.setColour (juce::Colour (0xff323c48));
            g.drawHorizontalLine ((int) Y (d), plot.getX(), plot.getRight());
            g.setColour (C::dim);
            g.drawText (juce::String ((int) d), juce::Rectangle<float> (0, Y (d) - 6, 24, 12), juce::Justification::centredRight);
        }
        for (float f : { 100.0f, 300.0f, 1000.0f, 3000.0f, 10000.0f })
        {
            g.setColour (juce::Colour (0xff323c48));
            g.drawVerticalLine ((int) X (f), plot.getY(), plot.getBottom());
            g.setColour (C::dim);
            g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f),
                        juce::Rectangle<float> (X (f) - 20, plot.getBottom() + 3, 40, 12), juce::Justification::centred);
        }

        // legenda
        auto leg = area.removeFromTop (18).withTrimmedRight (28);
        g.setFont (font (11.0f));
        auto drawLeg = [&] (const juce::String& t, juce::Colour c)
        {
            const float tw = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), t);
            auto r = leg.removeFromRight (tw + 30);
            g.setColour (c);
            g.fillRoundedRectangle (r.getX(), r.getCentreY() - 1.5f, 18, 3, 1.5f);
            g.setColour (C::dim);
            g.drawText (t, r.withTrimmedLeft (24), juce::Justification::centredLeft);
        };
        drawLeg (nameB, C::amber);
        drawLeg (nameA, juce::Colour (0xff7fe6d4));

        if (! hasData)
        {
            g.setColour (C::dim);
            g.setFont (font (13.0f));
            g.drawText (tr ("Fai suonare il brano per vedere lo spettro", "Play the song to see the spectrum"), plot, juce::Justification::centred);
            return;
        }

        float rel[kNumBands], dev[kNumBands], tg[kNumBands];
        Advisor::relativeBands (lng, rel);
        Advisor::deviation (lng, tgt, dev, -20.0f);
        const float offset = rel[0] - tgt[0] - dev[0];
        for (int b = 0; b < kNumBands; ++b) tg[b] = tgt[b] + offset;

        auto makePath = [&] (const float* vals, float add)
        {
            juce::Array<juce::Point<float>> pts;
            pts.add ({ X (20.0f), Y (vals[0] + add) });
            for (int b = 0; b < kNumBands; ++b) pts.add ({ X (kBandCentre[b]), Y (vals[b] + add) });
            pts.add ({ X (20000.0f), Y (vals[kNumBands - 1] + add) });
            juce::Path p;
            p.startNewSubPath (pts[0]);
            for (int i = 1; i < pts.size(); ++i)
            {
                const auto a = pts[i - 1], b = pts[i];
                const float mx = (a.x + b.x) * 0.5f;
                p.cubicTo (mx, a.y, mx, b.y, b.x, b.y);
            }
            return p;
        };

        // banda di tolleranza
        {
            juce::Path band (makePath (tg, tol));
            juce::Array<juce::Point<float>> downPts;
            for (int b = kNumBands - 1; b >= 0; --b) downPts.add ({ X (kBandCentre[b]), Y (tg[b] - tol) });
            band.lineTo (X (20000.0f), Y (tg[kNumBands - 1] - tol));
            for (auto& p : downPts) band.lineTo (p);
            band.lineTo (X (20.0f), Y (tg[0] - tol));
            band.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillPath (band);
        }

        auto mainPath = makePath (rel, 0.0f);
        juce::Path fill (mainPath);
        fill.lineTo (X (20000.0f), plot.getBottom());
        fill.lineTo (X (20.0f), plot.getBottom());
        fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (C::teal.withAlpha (0.75f), 0, plot.getY(), C::teal2.withAlpha (0.08f), 0, plot.getBottom(), false));
        g.fillPath (fill);
        g.setColour (juce::Colour (0xff7fe6d4));
        g.strokePath (mainPath, juce::PathStrokeType (2.0f));

        float mx = kSilenceDb;
        for (int b = 0; b < kNumBands; ++b) mx = juce::jmax (mx, lng[b]);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.strokePath (makePath (sht, -mx), juce::PathStrokeType (1.2f));

        juce::Path dashed;
        const float dashes[] = { 5.0f, 4.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, makePath (tg, 0.0f), dashes, 2);
        g.setColour (C::amber);
        g.fillPath (dashed);

        // punti cliccabili sulle discrepanze
        markerPos.clearQuick();
        for (int i = 0; i < markers.size(); ++i)
        {
            const auto& mk = markers.getReference (i);
            const juce::Point<float> p { X (kBandCentre[mk.band]), Y (rel[mk.band]) };
            markerPos.add (p);
            const auto col = C::severity (mk.severity);
            const float pulse = i == openMarker ? 13.0f : 11.0f;
            g.setColour (col.withAlpha (0.25f));
            g.fillEllipse (juce::Rectangle<float> (pulse * 2.2f, pulse * 2.2f).withCentre (p));
            g.setColour (col);
            g.fillEllipse (juce::Rectangle<float> (pulse * 1.4f, pulse * 1.4f).withCentre (p));
            g.setColour (juce::Colour (0xff0f1a1c));
            g.setFont (font (12.0f, true));
            g.drawText ("!", juce::Rectangle<float> (20, 20).withCentre (p), juce::Justification::centred);
        }

        // fumetto del punto aperto
        bubbleArea = {};
        linkArea = {};
        if (openMarker >= 0 && openMarker < markers.size())
        {
            const auto& mk = markers.getReference (openMarker);
            const auto p = markerPos[openMarker];
            const float bw = juce::jmin (330.0f, getWidth() - 20.0f);
            juce::AttributedString s;
            s.setWordWrap (juce::AttributedString::byWord);
            s.append (bandName (mk.band) + "\n", font (10.0f, true), C::severity (mk.severity));
            s.append (mk.title + "\n", font (13.0f, true), C::text);
            s.append (mk.detail.upToFirstOccurrenceOf (". ", true, false), font (12.0f), juce::Colour (0xffb9c4d0));
            juce::TextLayout tl;
            tl.createLayout (s, bw - 24.0f);
            const float bh = tl.getHeight() + 46.0f;
            float bx = juce::jlimit (6.0f, getWidth() - bw - 6.0f, p.x - bw * 0.5f);
            float by = p.y + 18.0f;
            if (by + bh > getHeight() - 4.0f) by = p.y - 18.0f - bh;
            bubbleArea = { bx, by, bw, bh };
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (bubbleArea.translated (0, 3), 10.0f);
            g.setColour (C::well);
            g.fillRoundedRectangle (bubbleArea, 10.0f);
            g.setColour (C::severity (mk.severity));
            g.drawRoundedRectangle (bubbleArea.reduced (0.5f), 10.0f, 1.2f);
            tl.draw (g, { bx + 12.0f, by + 10.0f, bw - 24.0f, tl.getHeight() });
            linkArea = { bx + 12.0f, by + bh - 30.0f, bw - 24.0f, 22.0f };
            g.setColour (C::teal);
            g.setFont (font (12.5f, true));
            g.drawText (tr ("Vai al consiglio  >", "Go to advice  >"), linkArea, juce::Justification::centredLeft);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.eventComponent != this) return;
        const auto p = e.position;
        if (! linkArea.isEmpty() && linkArea.expanded (4).contains (p) && openMarker >= 0)
        {
            const int band = markers[openMarker].band;
            openMarker = -1;
            repaint();
            if (onGoToAdvice) onGoToAdvice (band);
            return;
        }
        if (bubbleArea.contains (p)) return;
        int hit = -1;
        for (int i = 0; i < markerPos.size(); ++i)
            if (markerPos[i].getDistanceFrom (p) < 16.0f) hit = i;
        openMarker = (hit == openMarker) ? -1 : hit;
        repaint();
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        if (e.eventComponent != this) return;
        bool over = linkArea.contains (e.position);
        for (auto& mp : markerPos) if (mp.getDistanceFrom (e.position) < 16.0f) over = true;
        setMouseCursor (over ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

private:
    juce::Slider scale;
    juce::Array<Marker> markers;
    juce::Array<juce::Point<float>> markerPos;
    juce::Rectangle<float> bubbleArea, linkArea;
    int openMarker = -1;
    static juce::String bandName (int b) { return Advisor::bandName (b); }
    float lng[kNumBands] {}, sht[kNumBands] {}, tgt[kNumBands] {}, tol = 3;
    juce::String nameA, nameB;
    bool hasData = false;
};

//==============================================================================
// Pulsante da trascinare nella catena insert di Studio Pro: crea il preset nativo al momento del trascinamento.
// Clic semplice: crea il file e lo mostra nella cartella (da li' si puo' trascinare comunque).
class DragPresetButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<juce::File()> makeFile;   // scrive il preset e lo restituisce; File{} = nulla da esportare
    std::function<void()> onDragged;        // chiamato a trascinamento concluso
    juce::String label;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = isEnabled();
        g.setColour (on ? C::teal.withAlpha (over ? 0.30f : 0.18f) : C::panel2);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? C::teal : C::line);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
        // icona: maniglia di trascinamento (sei punti)
        auto ic = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.3f);
        g.setColour (on ? C::teal : C::dim);
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 3; ++j)
                g.fillEllipse (ic.getX() + i * ic.getWidth() * 0.6f, ic.getY() + j * ic.getHeight() * 0.42f, 2.6f, 2.6f);
        g.setColour (on ? C::text : C::dim);
        g.setFont (font (12.0f, true));
        g.drawFittedText (label, r.withTrimmedRight (10).toNearestInt(), juce::Justification::centredLeft, 1, 0.85f);
    }

    void mouseEnter (const juce::MouseEvent&) override { over = true; setMouseCursor (juce::MouseCursor::DraggingHandCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { over = false; repaint(); }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        const auto f = makeFile();
        if (! f.existsAsFile()) return;
        dragging = true;
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this,
            [safe = juce::Component::SafePointer<DragPresetButton> (this)]
            {
                if (safe == nullptr) return;
                safe->dragging = false;
                if (auto cb = safe->onDragged)
                    juce::MessageManager::callAsync (cb);   // mai dentro la callback del trascinamento
            });
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() >= 6 || ! makeFile) return;
        const auto f = makeFile();
        if (f.existsAsFile()) f.revealToUser();
    }

private:
    bool over = false, dragging = false;
};

//==============================================================================
// Lista dei consigli a schede
class AdviceList : public juce::Component
{
public:
    std::function<void (float)> onApplyLevel;                   // riceve il valore ASSOLUTO del trim
    std::function<void (const Advice&, bool)> onDoneChanged;
    std::function<juce::File (const Advice&)> onMakePreset;     // scrive il preset nativo con la correzione del consiglio
    std::function<void (const Advice&)> onPresetDragged;        // preset trascinato nella catena: segna Fatto e riparte l'analisi

    AdviceList()
    {
        addAndMakeVisible (viewport);
        viewport.setViewedComponent (&content, false);
        viewport.setScrollBarsShown (true, false);
        viewport.setScrollBarThickness (8);
    }

    void setAdvice (const juce::Array<Advice>& list, int columns = 1, bool allowApply = false)
    {
        // evita di ricostruire se nulla e' cambiato
        juce::String sig;
        for (auto& a : list) sig << a.severity << a.status << a.key << a.title << a.detail << a.plugin << "|";
        sig << columns << (int) allowApply << getWidth();
        if (sig == lastSig) return;
        lastSig = sig;
        items = list; cols = columns; apply = allowApply;
        layoutCards();
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());
        lastSig.clear();
        layoutCards();
    }

    // evidenzia e mostra la scheda collegata a una banda di frequenza
    void focusBand (int band)
    {
        highlightBand = band;
        for (auto* c : cards)
        {
            c->highlighted = c->a.band == band;
            c->repaint();
            if (c->highlighted)
                viewport.setViewPosition (0, juce::jmax (0, c->getY() - 6));
        }
    }

private:
    struct Card : public juce::Component
    {
        Advice a;
        juce::TextButton applyBtn;
        DragPresetButton presetBtn;
        juce::ToggleButton doneBtn;
        juce::TextLayout layout;
        float textH = 0;
        bool highlighted = false;

        juce::Colour accent() const
        {
            if (a.status == 3) return C::teal;
            if (a.status == 1) return C::dim;
            return C::severity (a.severity);
        }

        void build (int width, bool allowApply, std::function<void (float)> cb, std::function<void (const Advice&, bool)> doneCb,
                    std::function<juce::File (const Advice&)> makeCb, std::function<void (const Advice&)> draggedCb)
        {
            juce::String tag = a.category.toUpperCase();
            if (a.status == 1) tag << "  |  " << tr ("FATTO, DA VERIFICARE", "DONE, TO VERIFY");
            if (a.status == 2) tag << "  |  " << tr ("ANCORA PRESENTE", "STILL THERE");
            if (a.status == 3) tag = tr ("RISOLTO", "SOLVED");
            juce::AttributedString s;
            s.setWordWrap (juce::AttributedString::byWord);
            s.append (tag + "\n", font (10.0f, true), accent());
            s.append (a.title + "\n", font (13.5f, true), C::text);
            s.append (a.detail, font (12.5f), juce::Colour (0xffb9c4d0));
            if (a.plugin.isNotEmpty())
                s.append ("\n" + juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb8 ")) + a.plugin, font (12.0f), C::teal);
            s.setLineSpacing (2.0f);
            layout.createLayout (s, (float) width - 28.0f);
            textH = layout.getHeight();

            const bool levelApply = allowApply && std::isfinite (a.levelDelta) && std::isfinite (a.levelTarget);
            const int kind = presetKind (a.fix);
            const bool fixApply = allowApply && kind >= 0 && a.status != 3 && makeCb != nullptr;
            const bool showApply = levelApply || fixApply;
            applyBtn.setVisible (levelApply);
            presetBtn.setVisible (fixApply && ! levelApply);
            if (fixApply && ! levelApply)
            {
                const auto name = kind == 0 ? juce::String ("Pro EQ3") : juce::String ("Compressor");
                presetBtn.label = (a.status == 2 ? tr ("Trascina di nuovo ", "Drag again: ") : tr ("Trascina ", "Drag ")) + name;
                presetBtn.setTooltip (tr ("Trascina nella catena insert della traccia in Studio Pro, sopra May Pro Mix Track (se il plug-in c'e' gia', rilascialo sopra di lui per aggiornarlo). Clic: mostra il file del preset.",
                                          "Drag into the track insert chain in Studio Pro, above May Pro Mix Track (if the plug-in is already there, drop onto it to update it). Click: show the preset file."));
                const Advice copy = a;
                presetBtn.makeFile = [makeCb, copy] { return makeCb ? makeCb (copy) : juce::File(); };
                presetBtn.onDragged = [draggedCb, copy] { if (draggedCb) draggedCb (copy); };
                addAndMakeVisible (presetBtn);
            }
            else if (levelApply)
            {
                applyBtn.setButtonText (tr ("Applica ", "Apply ") + num (a.levelDelta, 1, true) + tr (" dB al Trim", " dB to Trim"));
                applyBtn.setToggleState (true, juce::dontSendNotification);
                const float target = a.levelTarget;
                applyBtn.onClick = [cb, target] { if (cb) cb (target); };
                addAndMakeVisible (applyBtn);
            }
            const bool showDone = a.checkable();
            doneBtn.setVisible (showDone);
            if (showDone)
            {
                doneBtn.setButtonText (tr ("Fatto", "Done"));
                doneBtn.setToggleState (a.status >= 1, juce::dontSendNotification);
                const Advice copy = a;
                doneBtn.onClick = [this, doneCb, copy] { if (doneCb) doneCb (copy, doneBtn.getToggleState()); };
                addAndMakeVisible (doneBtn);
            }
            const int extra = (showApply || showDone) ? 38 : 0;
            setSize (width, (int) std::ceil (textH) + 22 + extra);
            applyBtn.setBounds (14, (int) textH + 18, 190, 28);
            presetBtn.setBounds (14, (int) textH + 18, 210, 28);
            doneBtn.setBounds (width - 96, (int) textH + 18, 86, 28);
        }

        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (C::well);
            g.fillRoundedRectangle (r, 9.0f);
            g.setColour (highlighted ? C::teal : juce::Colour (0xff232c37));
            g.drawRoundedRectangle (r, 9.0f, highlighted ? 2.0f : 1.0f);
            g.setColour (accent());
            g.fillRoundedRectangle (0.5f, 8.0f, 3.0f, r.getHeight() - 16.0f, 1.5f);
            layout.draw (g, { 14.0f, 10.0f, r.getWidth() - 26.0f, textH });
        }
    };

    void layoutCards()
    {
        cards.clear();
        const int gap = 8;
        const int totalW = juce::jmax (100, getWidth() - 10);
        const int colW = (totalW - gap * (cols - 1)) / cols;
        juce::Array<int> colY;
        for (int c = 0; c < cols; ++c) colY.add (0);

        for (auto& a : items)
        {
            auto* card = cards.add (new Card());
            card->a = a;
            card->build (colW, apply, onApplyLevel, onDoneChanged, onMakePreset, onPresetDragged);
            card->highlighted = highlightBand >= 0 && a.band == highlightBand;
            int c = 0;
            for (int k = 1; k < cols; ++k) if (colY[k] < colY[c]) c = k;
            card->setTopLeftPosition (c * (colW + gap), colY[c]);
            colY.set (c, colY[c] + card->getHeight() + gap);
            content.addAndMakeVisible (card);
        }
        int h = 0;
        for (auto y : colY) h = juce::jmax (h, y);
        content.setSize (totalW, h);
    }

    juce::Viewport viewport;
    juce::Component content;
    juce::OwnedArray<Card> cards;
    juce::Array<Advice> items;
    juce::String lastSig;
    int cols = 1, highlightBand = -1;
    bool apply = false;
};

//==============================================================================
// Selettore lingua IT / EN
class LangSwitch : public juce::Component
{
public:
    std::function<void()> onChange;
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (C::panel2);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (C::line);
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        const bool en = SharedBus::get().english();
        auto left = r.reduced (3).removeFromLeft (r.getWidth() * 0.5f - 3), right = r.reduced (3).removeFromRight (r.getWidth() * 0.5f - 3);
        g.setColour (C::teal);
        g.fillRoundedRectangle (en ? right : left, left.getHeight() * 0.5f);
        g.setFont (font (12.0f, true));
        g.setColour (en ? C::dim : juce::Colour (0xff0f1a1c));
        g.drawText ("IT", left, juce::Justification::centred);
        g.setColour (en ? juce::Colour (0xff0f1a1c) : C::dim);
        g.drawText ("EN", right, juce::Justification::centred);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        SharedBus::get().setEnglish (e.x > getWidth() / 2);
        repaint();
        if (onChange) onChange();
    }
};

//==============================================================================
// Logo May Studio Recording in un riquadro scuro
inline void drawLogo (juce::Graphics& g, const juce::Image& logo, juce::Rectangle<int> r)
{
    g.setColour (juce::Colour (0xff0d1015));
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff1a2029));
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);
    if (logo.isValid())
        g.drawImageWithin (logo, r.getX() + 4, r.getY() + 3, r.getWidth() - 8, r.getHeight() - 6, juce::RectanglePlacement::centred);
}

inline void drawBrand (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& kind)
{
    g.setColour (C::text);
    g.setFont (font (18.0f, true));
    g.drawText ("May Pro Mix", r.removeFromTop (r.getHeight() / 2 + 4), juce::Justification::bottomLeft);
    g.setColour (C::teal);
    g.setFont (font (10.5f, true));
    g.drawText (kind, r, juce::Justification::topLeft);
}

// segno di spunta disegnato (non dipende dai font del sistema)
inline void drawCheck (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float thickness = 2.0f)
{
    juce::Path p;
    p.startNewSubPath (r.getX() + r.getWidth() * 0.18f, r.getY() + r.getHeight() * 0.52f);
    p.lineTo (r.getX() + r.getWidth() * 0.42f, r.getY() + r.getHeight() * 0.76f);
    p.lineTo (r.getX() + r.getWidth() * 0.82f, r.getY() + r.getHeight() * 0.26f);
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// pallino verde con spunta: "traccia corretta"
inline void drawOkBadge (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (C::teal);
    g.fillEllipse (r);
    drawCheck (g, r.reduced (r.getWidth() * 0.2f), juce::Colour (0xff0f1a1c), juce::jmax (1.6f, r.getWidth() * 0.11f));
}

inline void drawStat (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& value, const juce::String& label)
{
    g.setColour (C::well);
    g.fillRoundedRectangle (r.toFloat(), 8.0f);
    r = r.reduced (10, 7);
    g.setColour (C::text);
    g.setFont (font (15.0f, true));
    g.drawText (value, r.removeFromTop (20), juce::Justification::centredLeft);
    g.setColour (C::dim);
    g.setFont (font (9.5f, true));
    g.drawText (label.toUpperCase(), r, juce::Justification::centredLeft);
}
} // namespace mpm::ui
