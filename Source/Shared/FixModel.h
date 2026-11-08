#pragma once
#include "Advisor.h"
#include <cmath>

// Valori dei preset per i plug-in nativi di Studio Pro (Pro EQ3 e Compressor).
// May Pro Mix non elabora l'audio: prepara i valori e li consegna come preset da trascinare nella catena insert.
namespace mpm
{
struct FixSettings
{
    bool  enabled = true;
    bool  hpfOn = false;   float hpfFreq = 80.0f;  int hpfSlope = 12;
    bool  eqOn[3] {};      int eqType[3] {};       // 0 campana, 1 shelf basso, 2 shelf alto
    float eqFreq[3] { 200.0f, 1000.0f, 5000.0f }, eqGain[3] {}, eqQ[3] { 1.0f, 1.0f, 1.0f };
    bool  compOn = false;  float compThr = -18.0f, compRatio = 3.0f, compAtt = 10.0f, compRel = 100.0f, compMakeup = 0.0f;
    bool  deessOn = false; float deessFreq = 6500.0f, deessThr = -14.0f, deessRange = 6.0f;

    bool hasEq() const noexcept
    {
        bool any = hpfOn;
        for (int k = 0; k < 3; ++k) any = any || (eqOn[k] && std::abs (eqGain[k]) > 0.05f);
        return any;
    }
};

// quale plug-in nativo serve per una correzione: 0 Pro EQ3, 1 Compressor, -1 nessuno
inline int presetKind (const FixAction& f) noexcept
{
    switch (f.type)
    {
        case FixType::Hpf: case FixType::Eq:          return 0;
        case FixType::Comp: case FixType::CompSoften: return 1;
        default:                                      return -1;
    }
}

// valori letti dalla memoria condivisa (stesso ordine di FixStateIndex)
inline FixSettings settingsFromState (const float* v) noexcept
{
    auto f = [v] (int i, float def) { return finiteOr (v[i], def); };
    FixSettings s;
    s.hpfOn = f (fsHpfOn, 0) > 0.5f; s.hpfFreq = f (fsHpfFreq, 80); s.hpfSlope = f (fsHpfSlope, 0) > 0.5f ? 24 : 12;
    for (int k = 0; k < 3; ++k)
    {
        const int b = fsEq0On + k * 5;
        s.eqOn[k] = f (b, 0) > 0.5f; s.eqType[k] = juce::jlimit (0, 2, juce::roundToInt (f (b + 1, 0)));
        s.eqFreq[k] = f (b + 2, 1000); s.eqGain[k] = f (b + 3, 0); s.eqQ[k] = f (b + 4, 1);
    }
    s.compOn = f (fsCompOn, 0) > 0.5f; s.compThr = f (fsCompThr, -18); s.compRatio = f (fsCompRatio, 3);
    s.compAtt = f (fsCompAtt, 10); s.compRel = f (fsCompRel, 100); s.compMakeup = f (fsCompMakeup, 0);
    return s;
}

// aggiunge una correzione ai valori esistenti (le correzioni si sommano nello stesso preset)
inline void applyFixTo (FixSettings& s, const FixAction& f) noexcept
{
    switch (f.type)
    {
        case FixType::Hpf:
            s.hpfOn = true;
            s.hpfFreq = juce::jlimit (20.0f, 600.0f, f.freq);
            s.hpfSlope = f.slope >= 24 ? 24 : 12;
            break;

        case FixType::Eq:
        {
            // stessa zona di una banda attiva -> la aggiorno; altrimenti la prima libera; altrimenti la meno incisiva
            int k = -1;
            for (int i = 0; i < 3 && k < 0; ++i)
                if (s.eqOn[i] && s.eqType[i] == f.shape
                    && std::abs (std::log2 (juce::jmax (1.0f, s.eqFreq[i]) / juce::jmax (1.0f, f.freq))) < 0.6f)
                    k = i;
            const bool update = k >= 0;
            for (int i = 0; i < 3 && k < 0; ++i) if (! s.eqOn[i]) k = i;
            if (k < 0)
            {
                k = 0;
                for (int i = 1; i < 3; ++i) if (std::abs (s.eqGain[i]) < std::abs (s.eqGain[k])) k = i;
            }
            s.eqGain[k] = juce::jlimit (-15.0f, 15.0f, update ? s.eqGain[k] + f.gain : f.gain);
            s.eqOn[k] = true;
            s.eqType[k] = f.shape;
            s.eqFreq[k] = juce::jlimit (20.0f, 20000.0f, f.freq);
            s.eqQ[k] = juce::jlimit (0.3f, 8.0f, f.q);
            break;
        }

        case FixType::Comp:
            s.compOn = true;
            s.compThr = juce::jlimit (-60.0f, 0.0f, f.thr);
            s.compRatio = juce::jlimit (1.0f, 20.0f, f.ratio);
            s.compAtt = juce::jlimit (0.5f, 200.0f, f.attack);
            s.compRel = juce::jlimit (10.0f, 2000.0f, f.release);
            s.compMakeup = juce::jlimit (0.0f, 18.0f, f.makeup);
            break;

        case FixType::CompSoften:
            if (s.compOn)
            {
                s.compThr = juce::jmin (0.0f, s.compThr + 4.0f);
                s.compRatio = juce::jmax (1.5f, s.compRatio * 0.75f);
                s.compMakeup = juce::jmax (0.0f, s.compMakeup - 2.0f);
            }
            break;

        default: break;
    }
}
} // namespace mpm
