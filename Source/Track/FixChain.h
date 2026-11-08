#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

// Correzione integrata di May Pro Mix Track: passa-alto, 3 bande di EQ, compressore, de-esser.
// Tutto senza allocazioni nel thread audio; i coefficienti si ricalcolano solo quando cambiano i parametri.
namespace mpm
{
struct FixSettings
{
    bool  enabled = false;
    bool  hpfOn = false;   float hpfFreq = 80.0f;  int hpfSlope = 12;
    bool  eqOn[3] {};      int eqType[3] {};       // 0 campana, 1 shelf basso, 2 shelf alto
    float eqFreq[3] { 200.0f, 1000.0f, 5000.0f }, eqGain[3] {}, eqQ[3] { 1.0f, 1.0f, 1.0f };
    bool  compOn = false;  float compThr = -18.0f, compRatio = 3.0f, compAtt = 10.0f, compRel = 100.0f, compMakeup = 0.0f;
    bool  deessOn = false; float deessFreq = 6500.0f, deessThr = -14.0f, deessRange = 6.0f;
};

class FixChain
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        reset();
        dirty = true;
    }

    void reset() noexcept
    {
        for (auto* b : { &hp1, &hp2, &eq[0], &eq[1], &eq[2], &dsDetect, &dsSplit }) b->clear();
        compEnv = 0; compGr = 0; dsHigh = dsFull = 0; dsGr = 0;
    }

    void setSettings (const FixSettings& s) noexcept
    {
        if (! same (s, cur)) { cur = s; dirty = true; }
    }

    void process (float* const* ch, int numCh, int n) noexcept
    {
        if (! cur.enabled || n <= 0 || numCh <= 0) { grComp.store (0); grDeess.store (0); return; }
        numCh = juce::jmin (numCh, 2);
        if (dirty) updateCoefficients();

        // filtri lineari
        for (int c = 0; c < numCh; ++c)
        {
            float* x = ch[c];
            for (int i = 0; i < n; ++i)
            {
                double v = x[i];
                if (! std::isfinite (v)) v = 0.0;
                if (cur.hpfOn)
                {
                    v = hp1.run (v, c);
                    if (cur.hpfSlope >= 24) v = hp2.run (v, c);
                }
                for (int k = 0; k < 3; ++k)
                    if (cur.eqOn[k] && std::abs (cur.eqGain[k]) > 0.01f) v = eq[k].run (v, c);
                x[i] = (float) v;
            }
        }

        // de-esser: rapporto tra energia nella banda delle sibilanti ed energia totale (indipendente dal livello)
        if (cur.deessOn && cur.deessRange > 0.1f)
        {
            float maxRed = 0;
            for (int i = 0; i < n; ++i)
            {
                double m = 0;
                for (int c = 0; c < numCh; ++c) m += ch[c][i];
                m /= numCh;
                const double hb = dsDetect.run (m, 0);
                dsHigh += dsCoef * (hb * hb - dsHigh);
                dsFull += dsCoef * (m * m - dsFull);
                const double ratioDb = 10.0 * std::log10 ((dsHigh + 1.0e-12) / (dsFull + 1.0e-12));
                const double over = ratioDb - cur.deessThr;
                const double target = juce::jlimit (0.0, (double) cur.deessRange, over * 1.5);
                dsGr += (target > dsGr ? dsAtt : dsRelC) * (target - dsGr);
                const double g = std::pow (10.0, -dsGr / 20.0);
                for (int c = 0; c < numCh; ++c)
                {
                    const double x = ch[c][i];
                    const double hi = dsSplit.run (x, c);          // banda alta (passa-alto)
                    ch[c][i] = (float) (x - (1.0 - g) * hi);        // con g = 1 il segnale resta identico
                }
                maxRed = juce::jmax (maxRed, (float) dsGr);
            }
            grDeess.store (maxRed);
        }
        else grDeess.store (0);

        // compressore RMS con ginocchio morbido
        if (cur.compOn && cur.compRatio > 1.01f)
        {
            const double makeup = std::pow (10.0, cur.compMakeup / 20.0);
            float maxGr = 0;
            for (int i = 0; i < n; ++i)
            {
                double p = 0;
                for (int c = 0; c < numCh; ++c) p = juce::jmax (p, (double) ch[c][i] * ch[c][i]);
                compEnv += rmsCoef * (p - compEnv);
                const double lvl = 10.0 * std::log10 (compEnv + 1.0e-12);
                const double over = lvl - cur.compThr;
                const double knee = 6.0, slope = 1.0 - 1.0 / cur.compRatio;
                double target = 0;
                if (over > knee * 0.5) target = over * slope;
                else if (over > -knee * 0.5) { const double t = over + knee * 0.5; target = slope * t * t / (2.0 * knee); }
                compGr += (target > compGr ? attCoef : relCoef) * (target - compGr);
                const double g = std::pow (10.0, -compGr / 20.0) * makeup;
                for (int c = 0; c < numCh; ++c) ch[c][i] = (float) (ch[c][i] * g);
                maxGr = juce::jmax (maxGr, (float) compGr);
            }
            grComp.store (maxGr);
        }
        else { compGr = 0; grComp.store (0); }
    }

    std::atomic<float> grComp { 0 }, grDeess { 0 };   // riduzione di guadagno per i meter (dB)

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1[2] {}, z2[2] {};
        inline double run (double x, int c) noexcept
        {
            const double y = b0 * x + z1[c];
            z1[c] = b1 * x - a1 * y + z2[c];
            z2[c] = b2 * x - a2 * y;
            return y;
        }
        void clear() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
        void set (double nb0, double nb1, double nb2, double a0, double na1, double na2) noexcept
        {
            b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
        }
    };

    // ricette RBJ (Audio EQ Cookbook)
    void setHighPass (Biquad& f, double hz, double q) const noexcept
    {
        const double w = juce::MathConstants<double>::twoPi * hz / sr, cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        f.set ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
    }
    void setBandPass (Biquad& f, double hz, double q) const noexcept
    {
        const double w = juce::MathConstants<double>::twoPi * hz / sr, cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        f.set (al, 0, -al, 1 + al, -2 * cw, 1 - al);
    }
    void setPeak (Biquad& f, double hz, double q, double db) const noexcept
    {
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * hz / sr;
        const double cw = std::cos (w), al = std::sin (w) / (2.0 * q);
        f.set (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
    }
    void setShelf (Biquad& f, double hz, double db, bool high) const noexcept
    {
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * hz / sr;
        const double cw = std::cos (w), al = std::sin (w) / 2.0 * std::sqrt (2.0), sA = 2.0 * std::sqrt (A) * al;
        if (high)
            f.set (A * ((A + 1) + (A - 1) * cw + sA), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - sA),
                   (A + 1) - (A - 1) * cw + sA, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - sA);
        else
            f.set (A * ((A + 1) - (A - 1) * cw + sA), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - sA),
                   (A + 1) + (A - 1) * cw + sA, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - sA);
    }

    double clampHz (double hz) const noexcept { return juce::jlimit (10.0, sr * 0.45, hz); }

    void updateCoefficients() noexcept
    {
        dirty = false;
        if (cur.hpfSlope >= 24) { setHighPass (hp1, clampHz (cur.hpfFreq), 0.5412); setHighPass (hp2, clampHz (cur.hpfFreq), 1.3066); }
        else                     setHighPass (hp1, clampHz (cur.hpfFreq), 0.7071);
        for (int k = 0; k < 3; ++k)
        {
            const double f = clampHz (cur.eqFreq[k]), g = juce::jlimit (-18.0f, 18.0f, cur.eqGain[k]);
            if (cur.eqType[k] == 1)      setShelf (eq[k], f, g, false);
            else if (cur.eqType[k] == 2) setShelf (eq[k], f, g, true);
            else                         setPeak (eq[k], f, juce::jlimit (0.1f, 12.0f, cur.eqQ[k]), g);
        }
        setBandPass (dsDetect, clampHz (cur.deessFreq), 1.4);
        setHighPass (dsSplit, clampHz (cur.deessFreq * 0.7), 0.7071);

        auto coef = [this] (double ms) { return 1.0 - std::exp (-1.0 / (juce::jmax (0.1, ms) * 0.001 * sr)); };
        rmsCoef = coef (8.0);
        attCoef = coef (cur.compAtt);
        relCoef = coef (cur.compRel);
        dsCoef = coef (5.0);
        dsAtt = coef (1.0);
        dsRelC = coef (60.0);
    }

    static bool same (const FixSettings& a, const FixSettings& b) noexcept
    {
        if (a.enabled != b.enabled || a.hpfOn != b.hpfOn || a.hpfFreq != b.hpfFreq || a.hpfSlope != b.hpfSlope) return false;
        for (int k = 0; k < 3; ++k)
            if (a.eqOn[k] != b.eqOn[k] || a.eqType[k] != b.eqType[k] || a.eqFreq[k] != b.eqFreq[k] || a.eqGain[k] != b.eqGain[k] || a.eqQ[k] != b.eqQ[k])
                return false;
        return a.compOn == b.compOn && a.compThr == b.compThr && a.compRatio == b.compRatio && a.compAtt == b.compAtt
            && a.compRel == b.compRel && a.compMakeup == b.compMakeup && a.deessOn == b.deessOn && a.deessFreq == b.deessFreq
            && a.deessThr == b.deessThr && a.deessRange == b.deessRange;
    }

    double sr = 48000.0;
    FixSettings cur;
    bool dirty = true;
    Biquad hp1, hp2, eq[3], dsDetect, dsSplit;
    double compEnv = 0, compGr = 0, rmsCoef = 0.01, attCoef = 0.01, relCoef = 0.001;
    double dsHigh = 0, dsFull = 0, dsGr = 0, dsCoef = 0.01, dsAtt = 0.1, dsRelC = 0.001;
};
} // namespace mpm
