#pragma once
#include "Common.h"
#include <juce_data_structures/juce_data_structures.h>
#include <type_traits>

// Memoria condivisa tra tutte le istanze di May Pro Mix (Track e Master),
// anche se caricate da plug-in diversi o in processi diversi.
namespace mpm
{
// Consiglio segnato come "Fatto" dall'utente
struct DoneMark
{
    char key[32];
    char title[96];
    juce::uint32 epoch;      // epoca di analisi al momento della spunta
    juce::uint32 used;
};
constexpr int kMaxDone = 12;

struct SlotData
{
    juce::int64 lastSeenMs;
    juce::uint32 ownerId;
    juce::int32 role;          // ruolo effettivo (mai Auto)
    juce::uint32 colour;       // ARGB del colore traccia (0 = sconosciuto)
    juce::int32 numChannels;
    juce::uint32 resetAck;
    char name[64];

    float trimDb;
    float shortLufs, momentaryLufs, integratedLufs;
    float recentPeakDb, maxPeakDb, crestDb;
    float correlation, lowCorrelation, activeSeconds;
    float bandsLong[kNumBands];
    float bandsShort[kNumBands];
    juce::uint32 resetEpoch;   // aumenta a ogni azzeramento dell'analisi della traccia
    juce::uint32 trimRequestSeq;   // richiesta di variazione del trim inviata dal Master
    float trimRequestDb;
    juce::int32 excluded;          // 1 = esclusa dall'analisi (interruttore o plug-in in bypass)
    juce::uint32 excludeRequestSeq;
    juce::int32 excludeRequestValue;
    DoneMark done[kMaxDone];
    float reserved[16];
};

struct HeaderData
{
    juce::uint32 magic, version;
    juce::uint32 resetCounter;
    juce::int32 genre, loudTarget, refValid, language;   // language: 0 = italiano, 1 = english
    juce::int32 useReference;
    juce::int64 masterLastSeen;

    float masterShort, masterMomentary, masterIntegrated;
    float masterRecentPeak, masterMaxPeak, masterCrest;
    float masterCorrelation, masterLowCorrelation, masterActiveSeconds;
    float masterBandsLong[kNumBands], masterBandsShort[kNumBands];
    float refBands[kNumBands];
    float refLufs;
    DoneMark mixDone[kMaxDone];
    float reserved[32];
};

struct BusLayout
{
    HeaderData header;
    SlotData slots[kMaxSlots];
};
static_assert (std::is_trivially_copyable<BusLayout>::value, "BusLayout deve restare POD");

class SharedBus
{
public:
    static SharedBus& get();

    BusLayout& bus() noexcept          { return *layout; }
    bool isInterProcess() const noexcept { return interProcess; }

    int  claimSlot (juce::uint32 ownerId);
    void releaseSlot (int index, juce::uint32 ownerId);

    // orologio monotono condiviso da tutti i processi (non salta con l'ora legale o la sincronizzazione)
    static juce::int64 now() noexcept  { return (juce::int64) juce::Time::getMillisecondCounterHiRes(); }
    static bool isFresh (juce::int64 t, int ms = 5000) noexcept { return t > 0 && now() - t < ms; }

    // spunte "Fatto": slot = -1 per i consigli del mix
    void setDone (int slot, const juce::String& key, const juce::String& title, bool done);
    DoneMark* marksFor (int slot) noexcept;
    juce::uint32 epochFor (int slot) const noexcept;

    bool english() const noexcept      { return layout->header.language == 1; }
    void setEnglish (bool en);

    // impostazioni utente (tutorial visto, lingua preferita)
    static std::unique_ptr<juce::PropertiesFile> settings();

private:
    SharedBus();
    ~SharedBus() = default;     // volutamente mai distrutto: la memoria resta valida fino all'uscita dell'host
    void initialiseIfNeeded();

    BusLayout* layout = nullptr;
    bool interProcess = false;
    void* nativeHandle = nullptr;
    juce::InterProcessLock lock { "MayProMixBusLock" };
    JUCE_DECLARE_NON_COPYABLE (SharedBus)
};

// Testo bilingue: italiano / inglese
inline juce::String tr (const juce::String& it, const juce::String& en)
{
    return SharedBus::get().english() ? en : it;
}

// Numero formattato con la virgola in italiano e il punto in inglese
inline juce::String num (float v, int decimals = 1, bool plus = false)
{
    auto s = juce::String (v, decimals);
    if (plus && v > 0.0f) s = "+" + s;
    return SharedBus::get().english() ? s : s.replaceCharacter ('.', ',');
}
} // namespace mpm
