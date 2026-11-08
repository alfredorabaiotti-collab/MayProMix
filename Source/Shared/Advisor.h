#pragma once
#include "SharedBus.h"
#include "PluginCatalog.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Motore dei consigli di May Pro Mix
namespace mpm
{
struct TrackInfo
{
    int slot = -1;
    juce::String name;
    Role role = Role::Other;
    juce::Colour colour;
    int numChannels = 2;
    float trimDb = 0;
    // valori gia' comprensivi del trim
    float shortLufs = kSilenceDb, integratedLufs = kSilenceDb, recentPeakDb = kSilenceDb, maxPeakDb = kSilenceDb;
    float crestDb = 0, correlation = 1, lowCorrelation = 1, activeSeconds = 0;
    float bandsLong[kNumBands] {}, bandsShort[kNumBands] {};
    float relLevel = mpm::kNaN, targetRel = mpm::kNaN, fullness = 0;
    float levelDelta = mpm::kNaN;   // correzione di livello consigliata (gia' normalizzata sul brano)
    float fixState[fsCount] {};      // stato della correzione integrata della traccia
    float rawIntegrated = kSilenceDb; // loudness prima del trim (per calcolare la soglia del compressore)
    bool excluded = false;      // esclusa dall'analisi (mute segnalato dall'utente o plug-in in bypass)

    bool analysed() const noexcept { return ! excluded && activeSeconds >= 8.0f && integratedLufs > -70.0f; }
};

struct MixInfo
{
    bool masterPresent = false;
    float shortLufs = kSilenceDb, integratedLufs = kSilenceDb, recentPeakDb = kSilenceDb, maxPeakDb = kSilenceDb;
    float crestDb = 0, correlation = 1, lowCorrelation = 1, activeSeconds = 0;
    float bandsLong[kNumBands] {}, bandsShort[kNumBands] {};
    int genre = 0, loudTarget = 0;
    bool refValid = false;
    float refBands[kNumBands] {}, refLufs = kSilenceDb;
    float tracksSumLufs = kSilenceDb, fullness = 0;

    bool analysed() const noexcept { return masterPresent && activeSeconds >= 10.0f && integratedLufs > -70.0f; }
    const float* targetCurve() const noexcept { return refValid ? refBands : genreCurve (genre); }
};

struct Conflict
{
    int a = -1, b = -1, band = 0;
    float score = 0;
};

struct Snapshot
{
    juce::Array<TrackInfo> tracks;
    MixInfo mix;
    juce::Array<Conflict> conflictList;   // calcolate una volta per istantanea
    bool connected = true;                // false se la memoria condivisa non e' disponibile
    bool balanceReady = false;            // almeno 3 tracce analizzate con un riferimento di livello
    int indexOfSlot (int slot) const;
};

// Azione eseguibile con un clic sulla correzione integrata di May Pro Mix Track
enum class FixType : int { None = 0, Hpf, Eq, Comp, CompSoften, DeEss };

struct FixAction
{
    FixType type = FixType::None;
    float freq = 0, gain = 0, q = 1;
    int shape = 0;              // 0 campana, 1 shelf basso, 2 shelf alto
    int slope = 12;
    float thr = 0, ratio = 1, attack = 10, release = 100, makeup = 0, range = 0;

    bool valid() const noexcept { return type != FixType::None; }
    void toArray (float* d) const noexcept
    {
        const float v[16] { (float) type, freq, gain, q, (float) shape, (float) slope, thr, ratio, attack, release, makeup, range, 0, 0, 0, 0 };
        std::copy (v, v + 16, d);
    }
    static FixAction fromArray (const float* d) noexcept
    {
        FixAction f;
        const int t = juce::roundToInt (finiteOr (d[0], 0.0f));
        f.type = (t >= 0 && t <= (int) FixType::DeEss) ? (FixType) t : FixType::None;
        f.freq = finiteOr (d[1], 0); f.gain = finiteOr (d[2], 0); f.q = finiteOr (d[3], 1);
        f.shape = juce::jlimit (0, 2, juce::roundToInt (finiteOr (d[4], 0))); f.slope = finiteOr (d[5], 12) >= 24 ? 24 : 12;
        f.thr = finiteOr (d[6], 0); f.ratio = finiteOr (d[7], 1); f.attack = finiteOr (d[8], 10); f.release = finiteOr (d[9], 100);
        f.makeup = finiteOr (d[10], 0); f.range = finiteOr (d[11], 0);
        return f;
    }
};

struct Advice
{
    int severity = 0;              // 0 = informazione / ok, 1 = suggerimento, 2 = importante
    juce::String category, title, detail, plugin;
    float levelDelta = mpm::kNaN;        // per i consigli di livello: dB da aggiungere al trim
    float levelTarget = mpm::kNaN;       // valore assoluto del trim da impostare (sicuro anche se cliccato due volte)
    int band = -1;                 // banda di frequenza a cui si riferisce (per i punti sul grafico)
    juce::String key;              // identificativo stabile ("_" iniziale = non spuntabile)
    int status = 0;                // 0 normale, 1 fatto (da verificare), 2 ancora presente, 3 risolto
    FixAction fix;                 // se valida: il pulsante "Applica" la imposta nella correzione integrata
    bool checkable() const { return key.isNotEmpty() && ! key.startsWith ("_"); }
};

class Advisor
{
public:
    static Snapshot capture();
    static juce::Array<Advice> forTrack (const Snapshot&, int trackIndex);
    static juce::Array<Advice> forMix (const Snapshot&);
    static juce::Array<Conflict> conflicts (const Snapshot&);

    // applica le spunte "Fatto": verifica se il problema persiste e aggiunge un consiglio di secondo livello
    static void applyDoneMarks (juce::Array<Advice>& list, int slot, bool analysed);
    static juce::String followUp (const juce::String& key);

    static float trackFullness (const float* bands, float crest, Role role);
    static float mixFullness (const MixInfo&);

    // normalizzazione: banda relativa al massimo e scostamento dalla curva obiettivo
    static void relativeBands (const float* bands, float* rel);
    static void deviation (const float* bands, const float* target, float* dev, float coreThreshold = -12.0f);

    static juce::String roleName (Role);
    static juce::StringArray roleChoices();
    static void fillRoleCombo (juce::ComboBox& box);   // con le intestazioni Base / Orchestra / Pad
    static juce::String bandName (int band);
    static juce::String freqText (float hz);
    static juce::String fullnessLabel (float f);
    static juce::StringArray genreChoices();
    static juce::StringArray targetChoices();
};
} // namespace mpm
