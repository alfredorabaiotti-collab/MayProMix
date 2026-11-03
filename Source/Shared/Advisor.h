#pragma once
#include "SharedBus.h"
#include "PluginCatalog.h"
#include <juce_graphics/juce_graphics.h>

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
    int indexOfSlot (int slot) const;
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
    static juce::String bandName (int band);
    static juce::String freqText (float hz);
    static juce::String fullnessLabel (float f);
    static juce::StringArray genreChoices();
    static juce::StringArray targetChoices();
};
} // namespace mpm
