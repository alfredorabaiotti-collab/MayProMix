#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <limits>

// May Pro Mix: definizioni comuni (bande, ruoli, profili di riferimento)
namespace mpm
{
constexpr int kNumBands = 8;
constexpr int kMaxSlots = 96;
constexpr float kSilenceDb = -100.0f;
inline const float kNaN = std::numeric_limits<float>::quiet_NaN();

constexpr float kBandEdges[kNumBands + 1] = { 20.0f, 60.0f, 120.0f, 250.0f, 500.0f, 2000.0f, 4000.0f, 8000.0f, 20000.0f };
constexpr float kBandCentre[kNumBands]    = { 40.0f, 90.0f, 180.0f, 350.0f, 1000.0f, 3000.0f, 6000.0f, 12000.0f };

enum class Role : int
{
    Auto = 0, LeadVocal, BackingVocal, Kick, Snare, Overheads, DrumBus, Bass,
    RhythmGuitar, AcousticGuitar, LeadGuitar, Keys, Pad, Percussion, Other, Group, NumRoles
};
constexpr int kNumRoles = (int) Role::NumRoles;

enum LoudTarget { targetGenre = 0, targetStreaming, targetLoud, targetPreMaster, kNumTargets };

// Profilo di un ruolo. Bande espresse in dB per ottava rispetto alla banda piu' forte (0 dB).
struct RoleProfile
{
    float bands[kNumBands];
    float hpfHz;          // passa-alto consigliato (0 = nessuno)
    float crestMin, crestMax;
    int   priority;       // importanza nel mix (piu' alto = piu' importante)
    float level;          // LU rispetto alla somma delle tracce (base rock, poi corretto dal genere)
    const char* ratio;
    int   attackMs, releaseMs;
};

inline const RoleProfile& profileFor (Role r)
{
    static const RoleProfile p[kNumRoles] =
    {
        /* Auto       */ { { -30, -18, -8, -4, 0, -4, -9, -16 },  0, 8, 16, 1, kNaN,      "3:1", 15, 100 },
        /* LeadVocal  */ { { -40, -24, -10, -4, 0, -3, -8, -16 }, 90, 9, 14, 10, -4.0f, "3:1", 10, 80 },
        /* Backing    */ { { -45, -28, -12, -5, 0, -3, -7, -14 }, 140, 8, 13, 4, -12.0f, "4:1", 5, 60 },
        /* Kick       */ { { -4, 0, -6, -14, -16, -14, -16, -24 }, 30, 10, 18, 9, -8.0f, "4:1", 25, 60 },
        /* Snare      */ { { -24, -12, -2, -2, 0, -4, -6, -12 },  90, 12, 20, 8, -8.0f, "4:1", 15, 80 },
        /* Overheads  */ { { -36, -24, -14, -10, -4, -2, 0, -2 }, 250, 14, 22, 5, -13.0f, "2:1", 30, 150 },
        /* DrumBus    */ { { -10, -2, -2, -6, -4, -4, -4, -8 },   30, 10, 16, 8, -5.0f, "4:1", 30, 100 },
        /* Bass       */ { { -4, 0, -2, -6, -10, -18, -26, -34 }, 35, 6, 11, 7, -7.0f, "4:1", 15, 120 },
        /* RhythmGtr  */ { { -36, -18, -6, -2, 0, -2, -8, -20 }, 100, 6, 12, 5, -6.0f, "3:1", 20, 120 },
        /* Acoustic   */ { { -36, -18, -6, -4, 0, -2, -4, -10 }, 90, 10, 16, 6, -9.0f, "3:1", 10, 100 },
        /* LeadGtr    */ { { -40, -22, -8, -3, 0, -2, -8, -20 }, 110, 8, 13, 7, -7.0f, "3:1", 15, 100 },
        /* Keys       */ { { -24, -10, -4, -2, 0, -6, -12, -20 }, 60, 9, 15, 4, -11.0f, "2:1", 20, 150 },
        /* Pad        */ { { -20, -10, -4, -2, 0, -4, -8, -14 }, 120, 6, 14, 2, -14.0f, "2:1", 30, 200 },
        /* Percussion */ { { -30, -18, -10, -6, -2, 0, 0, -6 },  150, 10, 18, 3, -15.0f, "3:1", 10, 80 },
        /* Other      */ { { -30, -18, -8, -4, 0, -4, -9, -16 },  0, 8, 16, 2, kNaN,      "3:1", 15, 100 },
        /* Group      */ { { -10, -2, -2, -4, -2, -4, -6, -10 },  0, 6, 16, 0, kNaN,      "2:1", 30, 150 },
    };
    return p[juce::jlimit (0, kNumRoles - 1, (int) r)];
}

// Profili di genere: curva tonale del mix, loudness tipica, correzioni di livello per famiglia
// (voce, batteria, basso, chitarre elettriche, acustiche, tastiere), compressione e consiglio pratico.
struct GenreProfile
{
    const char* nameIt; const char* nameEn;
    float curve[kNumBands];
    float lufs;
    float offset[6];
    float crestShift;     // negativo = genere piu' compresso
    const char* tipIt; const char* tipEn;
};

constexpr int kNumGenres = 14;

inline const GenreProfile& genreProfile (int g)
{
    static const GenreProfile p[kNumGenres] =
    {
        { "Rock", "Rock", { -5, 0, -1, -3, -6, -9, -12, -17 }, -10, { 0, 0, 0, 0, 0, 0 }, 0,
          "Chitarre ritmiche raddoppiate e aperte a sinistra e destra, voce davanti ma dentro il muro di chitarre, rullante protagonista.",
          "Double-tracked rhythm guitars panned left and right, vocal on top but inside the guitar wall, snare as a lead element." },
        { "Hard rock / Metal", "Hard rock / Metal", { -4, 0, -2, -4, -6, -8, -11, -16 }, -8, { 0.5f, 1, 0, 2, -2, -2 }, -2,
          "Chitarre registrate 4 volte, basso con un po' di saturazione per farsi sentire, cassa con il click a 3-5 kHz, compressione parallela sulla batteria.",
          "Quad-tracked guitars, bass with some saturation to cut through, kick click at 3-5 kHz, parallel compression on drums." },
        { "Pop", "Pop", { -4, 0, -2, -4, -6, -8, -10, -13 }, -9, { 1.5f, 0, 0, -2, -1, 1 }, -2,
          "Voce sempre in primo piano e molto controllata, bassi puliti e mono, alti ariosi.",
          "Vocal always upfront and tightly controlled, clean mono low end, airy top." },
        { "Pop rock", "Pop rock", { -5, 0, -1, -3, -6, -8, -11, -15 }, -9, { 1, 0, 0, -1, 0, 0 }, -1,
          "Energia rock con la voce piu' avanti: chitarre larghe, ritornelli che si aprono anche con l'automazione.",
          "Rock energy with the vocal further forward: wide guitars, choruses that open up, also with automation." },
        { "Country", "Country", { -8, -2, 0, -2, -4, -7, -10, -14 }, -11, { 1, -1, -1, -2, 2, 0 }, 0,
          "Voce e acustica protagoniste, batteria naturale con poco riverbero, pedal steel o slide come contromelodia.",
          "Vocal and acoustic upfront, natural drums with little reverb, pedal steel or slide as a counter melody." },
        { "Country rock", "Country rock", { -7, -1, 0, -3, -5, -8, -11, -15 }, -10, { 0.5f, -0.5f, -1, -1, 1, 0 }, 0,
          "Il suono di Nashville: acustica e elettrica pulita che si incastrano, batteria asciutta, voce calda e vicina.",
          "The Nashville sound: acoustic and clean electric interlocking, dry drums, warm close vocal." },
        { "Blues", "Blues", { -7, -1, 0, -2, -4, -8, -11, -16 }, -12, { 0, -1, 0, 0.5f, 0, 0 }, 2,
          "Suono caldo e dinamico: comprimi poco, lascia respirare chitarra solista e voce, riverbero di stanza o plate.",
          "Warm, dynamic sound: compress lightly, let the lead guitar and vocal breathe, room or plate reverb." },
        { "Folk / Acustico", "Folk / Acoustic", { -10, -3, 0, -2, -4, -7, -10, -13 }, -13, { 1, -3, -2, -3, 2, 0 }, 3,
          "Pochi elementi: cura la naturalezza, compressione leggera, spazio con un riverbero di stanza.",
          "Few elements: keep it natural, light compression, space from a room reverb." },
        { "Ballata", "Ballad", { -8, -2, 0, -2, -4, -8, -11, -15 }, -12, { 1, -4, -2, -4, 3, 5 }, 2,
          "Voce e piano o acustica al centro, batteria discreta, riverberi lunghi ma con il pre-delay per non impastare la voce.",
          "Vocal and piano or acoustic at the centre, discreet drums, long reverbs with pre-delay so the vocal stays clear." },
        { "Cantautorato", "Singer-songwriter", { -9, -3, 0, -2, -4, -7, -10, -14 }, -13, { 1.5f, -3, -2, -3, 2, 2 }, 2,
          "Il testo e' il centro: voce chiara e intelligibile, arrangiamento al servizio della parola.",
          "The lyric is the centre: clear, intelligible vocal, arrangement serving the words." },
        { "Funk / Soul", "Funk / Soul", { -6, 0, -1, -3, -5, -8, -10, -14 }, -10, { 0, 0, 1, -1, -2, 1 }, 0,
          "Basso e cassa incastrati, chitarra ritmica pulita e secca, il groove prima di tutto.",
          "Bass and kick locked together, clean dry rhythm guitar, groove first." },
        { "Jazz", "Jazz", { -10, -3, 0, -2, -4, -8, -11, -14 }, -16, { 0, -3, 0, 0, 0, 2 }, 4,
          "Dinamica naturale: comprimi pochissimo, immagine stereo realistica come sul palco.",
          "Natural dynamics: compress very little, realistic stage-like stereo image." },
        { "Elettronica / Dance", "Electronic / Dance", { -1, 0, -3, -5, -7, -9, -11, -14 }, -7, { 0, 2, 1, -2, -3, 1 }, -3,
          "Cassa e sub dominanti e mono, sidechain sui pad e sui bassi, alti brillanti.",
          "Dominant mono kick and sub, sidechain on pads and bass, bright top end." },
        { "Hip hop", "Hip hop", { 0, 0, -3, -5, -7, -9, -11, -15 }, -8, { 2, 1, 1, -3, -3, 0 }, -2,
          "Voce dominante e asciutta, 808 e cassa che non si pestano i piedi, il resto attorno.",
          "Dominant dry vocal, 808 and kick that don't fight, everything else around them." },
    };
    return p[juce::jlimit (0, kNumGenres - 1, g)];
}

inline int roleFamily (Role r)
{
    switch (r)
    {
        case Role::LeadVocal: case Role::BackingVocal: return 0;
        case Role::Kick: case Role::Snare: case Role::Overheads: case Role::DrumBus: case Role::Percussion: return 1;
        case Role::Bass: return 2;
        case Role::RhythmGuitar: case Role::LeadGuitar: return 3;
        case Role::AcousticGuitar: return 4;
        case Role::Keys: case Role::Pad: return 5;
        default: return -1;
    }
}

inline const float* genreCurve (int genre) { return genreProfile (genre).curve; }

inline float targetLufs (int target, int genre)
{
    switch (target)
    {
        case targetGenre:     return genreProfile (genre).lufs;
        case targetStreaming: return -14.0f;
        case targetLoud:      return -8.0f;
        default:              return -20.0f;
    }
}

inline float dbToPow (float db)  { return std::pow (10.0f, db / 10.0f); }
inline float powToDb (double p)  { return (std::isfinite (p) && p > 1.0e-12) ? (float) juce::jmin (60.0, 10.0 * std::log10 (p)) : kSilenceDb; }
inline float finiteOr (float v, float fallback) noexcept { return std::isfinite (v) ? v : fallback; }

// Riconoscimento automatico del ruolo dal nome della traccia (italiano e inglese)
inline Role guessRoleFromName (const juce::String& rawName)
{
    auto n = rawName.toLowerCase();
    auto has = [&n] (std::initializer_list<const char*> keys)
    {
        for (auto k : keys) if (n.contains (k)) return true;
        return false;
    };

    if (n.isEmpty()) return Role::Other;
    if (has ({ "bus", "group", "gruppo", "submix" }))                         return Role::Group;
    if (has ({ "cori", "coro", "backing", "bvox", "bv ", "choir", "harmon", "armonizz" })) return Role::BackingVocal;
    if (has ({ "kick", "cassa", "bass drum", "bassdrum", "bd " }))           return Role::Kick;
    if (has ({ "snare", "rullante", "sn " }))                                return Role::Snare;
    if (has ({ "overhead", "oh ", "piatti", "cymbal", "hihat", "hi-hat", "hh", "ride", "crash" })) return Role::Overheads;
    if (has ({ "drum", "batteria", "kit", "room", "loop" }))                 return Role::DrumBus;
    if (has ({ "voce", "vox", "vocal", "voice", "lead voc", "canto" }))     return Role::LeadVocal;
    if (has ({ "bass", "basso" }))                                           return Role::Bass;
    if (has ({ "acoustic", "acustic", "acu " }))                             return Role::AcousticGuitar;
    if (has ({ "solo", "solista", "lead gtr", "lead guit" }))                return Role::LeadGuitar;
    if (has ({ "gtr", "guit", "chit", "rhythm", "ritmic", "mustang" }))     return Role::RhythmGuitar;
    if (has ({ "piano", "keys", "tastier", "organ", "organo", "rhodes", "wurli", "epiano" })) return Role::Keys;
    if (has ({ "pad", "synth", "string", "archi" }))                         return Role::Pad;
    if (has ({ "perc", "shaker", "tamb", "conga", "bongo", "clap" }))        return Role::Percussion;
    return Role::Other;
}
} // namespace mpm
