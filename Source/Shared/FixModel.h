#pragma once
#include "Advisor.h"
#include <cmath>

// Valori dei preset per i plug-in nativi di Studio Pro (Pro EQ3 e Compressor).
// May Pro Mix non elabora l'audio: prepara i valori e li consegna come preset da trascinare nella catena insert.
namespace mpm
{
constexpr int kEqBands = 5;
inline int eqShapeOf (int band) noexcept { return band == 0 ? 1 : band == kEqBands - 1 ? 2 : 0; }   // 0 campana, 1 shelf basso, 2 shelf alto

struct FixSettings
{
    bool  enabled = true;
    bool  hpfOn = false;   float hpfFreq = 80.0f;  int hpfSlope = 12;
    // le 5 bande del Pro EQ3: 0 LF (shelf bassi), 1-3 LMF/MF/HMF (campane), 4 HF (shelf alti)
    bool  eqOn[kEqBands] {};
    float eqFreq[kEqBands] { 100.0f, 400.0f, 1500.0f, 5000.0f, 10000.0f }, eqGain[kEqBands] {}, eqQ[kEqBands] { 0.7f, 1.0f, 1.0f, 1.0f, 0.7f };
    bool  compOn = false;  float compThr = -18.0f, compRatio = 3.0f, compAtt = 10.0f, compRel = 100.0f, compMakeup = 0.0f;
    bool  deessOn = false; float deessFreq = 6500.0f, deessThr = -14.0f, deessRange = 6.0f;

    bool hasEq() const noexcept
    {
        bool any = hpfOn;
        for (int k = 0; k < kEqBands; ++k) any = any || (eqOn[k] && std::abs (eqGain[k]) > 0.05f);
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
    for (int k = 0; k < kEqBands; ++k)
    {
        const int b = fsEq0On + k * 4;
        s.eqOn[k] = f (b, 0) > 0.5f; s.eqFreq[k] = f (b + 1, 1000); s.eqGain[k] = f (b + 2, 0); s.eqQ[k] = f (b + 3, 1);
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
            int k = -1;
            if (f.shape == 1) k = 0;
            else if (f.shape == 2) k = kEqBands - 1;
            float gain = f.gain, freq = f.freq, q = f.q;
            if (k >= 0)
            {
                if (s.eqOn[k]) gain += s.eqGain[k];   // stessa banda gia' attiva: le correzioni si sommano
            }
            else
            {
                // campane: stessa zona -> si sommano; altrimenti una libera; altrimenti si fonde con la piu' vicina
                auto dist = [&] (int i) { return std::abs (std::log2 (juce::jmax (1.0f, s.eqFreq[i]) / juce::jmax (1.0f, f.freq))); };
                for (int i = 1; i <= 3 && k < 0; ++i) if (s.eqOn[i] && dist (i) < 0.6f) k = i;
                if (k >= 0) gain += s.eqGain[k];
                for (int i = 1; i <= 3 && k < 0; ++i) if (! s.eqOn[i]) k = i;
                if (k < 0)
                {
                    // nessuna campana libera: fondo con la piu' vicina solo se va nella stessa direzione e a meno di 1,2 ottave,
                    // altrimenti tengo le correzioni piu' importanti (sostituisco la piu' debole se questa e' piu' forte)
                    int near = 1, weak = 1;
                    for (int i = 2; i <= 3; ++i)
                    {
                        if (dist (i) < dist (near)) near = i;
                        if (std::abs (s.eqGain[i]) < std::abs (s.eqGain[weak])) weak = i;
                    }
                    if (dist (near) < 1.2f && (s.eqGain[near] > 0) == (f.gain > 0))
                    {
                        k = near;
                        const float w0 = std::abs (s.eqGain[k]), w1 = std::abs (f.gain), wt = juce::jmax (0.01f, w0 + w1);
                        freq = std::exp2 ((std::log2 (s.eqFreq[k]) * w0 + std::log2 (f.freq) * w1) / wt);
                        q = juce::jmin (s.eqQ[k], f.q) * 0.8f;   // piu' larga per coprire le due zone
                        gain = juce::jmax (std::abs (s.eqGain[k]), std::abs (f.gain)) * (f.gain > 0 ? 1.0f : -1.0f);
                    }
                    else if (std::abs (f.gain) > std::abs (s.eqGain[weak]))
                        k = weak;
                    else
                        break;   // correzione meno importante di quelle gia' presenti: resta nel consiglio
                }
            }
            s.eqOn[k] = true;
            s.eqGain[k] = juce::jlimit (-15.0f, 15.0f, gain);
            s.eqFreq[k] = juce::jlimit (20.0f, 20000.0f, freq);
            s.eqQ[k] = juce::jlimit (0.3f, 8.0f, q);
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
// tutte le correzioni ancora da inserire per un plug-in (0 Pro EQ3, 1 Compressor): un solo trascinamento le porta tutte
inline juce::Array<Advice> pendingFor (const juce::Array<Advice>& list, int kind)
{
    juce::Array<Advice> out;
    for (const auto& a : list)
        if (presetKind (a.fix) == kind && a.status != 1 && a.status != 3)
            out.add (a);
    return out;
}

inline bool hasKey (const juce::Array<Advice>& list, const juce::String& key)
{
    for (const auto& a : list) if (a.key == key) return true;
    return false;
}

inline FixSettings withPending (FixSettings s, const juce::Array<Advice>& pending)
{
    for (const auto& a : pending) applyFixTo (s, a.fix);
    return s;
}

inline void toState (const FixSettings& s, float* v) noexcept
{
    v[fsEnabled] = 1.0f;
    v[fsHpfOn] = s.hpfOn ? 1.0f : 0.0f; v[fsHpfFreq] = s.hpfFreq; v[fsHpfSlope] = s.hpfSlope >= 24 ? 1.0f : 0.0f;
    for (int k = 0; k < kEqBands; ++k)
    {
        const int b = fsEq0On + k * 4;
        v[b] = s.eqOn[k] ? 1.0f : 0.0f; v[b + 1] = s.eqFreq[k]; v[b + 2] = s.eqGain[k]; v[b + 3] = s.eqQ[k];
    }
    v[fsCompOn] = s.compOn ? 1.0f : 0.0f; v[fsCompThr] = s.compThr; v[fsCompRatio] = s.compRatio;
    v[fsCompAtt] = s.compAtt; v[fsCompRel] = s.compRel; v[fsCompMakeup] = s.compMakeup;
    v[fsDeessOn] = 0.0f; v[fsDeessFreq] = s.deessFreq; v[fsDeessThr] = s.deessThr; v[fsDeessRange] = s.deessRange;
}
} // namespace mpm
