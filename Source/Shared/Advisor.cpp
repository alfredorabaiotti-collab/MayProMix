#include "Advisor.h"

namespace mpm
{
//==============================================================================
// Testi
juce::String Advisor::roleName (Role r)
{
    switch (r)
    {
        case Role::Auto:           return tr ("Automatico (dal nome)", "Automatic (from name)");
        case Role::LeadVocal:      return tr ("Voce principale", "Lead vocal");
        case Role::BackingVocal:   return tr ("Cori", "Backing vocals");
        case Role::Kick:           return tr ("Cassa", "Kick");
        case Role::Snare:          return tr ("Rullante", "Snare");
        case Role::Overheads:      return tr ("Overhead / piatti", "Overheads / cymbals");
        case Role::DrumBus:        return tr ("Batteria (stereo)", "Drums (stereo)");
        case Role::Bass:           return tr ("Basso", "Bass");
        case Role::RhythmGuitar:   return tr ("Chitarra ritmica", "Rhythm guitar");
        case Role::AcousticGuitar: return tr ("Chitarra acustica", "Acoustic guitar");
        case Role::LeadGuitar:     return tr ("Chitarra solista", "Lead guitar");
        case Role::Keys:           return tr ("Tastiere / piano", "Keys / piano");
        case Role::Pad:            return tr ("Pad / synth (generico)", "Pad / synth (generic)");
        case Role::Percussion:     return tr ("Percussioni", "Percussion");
        case Role::Other:          return tr ("Altro", "Other");
        case Role::Group:          return tr ("Bus / gruppo (escluso)", "Bus / group (excluded)");
        case Role::Violins:        return tr ("Violini", "Violins");
        case Role::Violas:         return tr ("Viole", "Violas");
        case Role::Cellos:         return tr ("Violoncelli", "Cellos");
        case Role::DoubleBasses:   return tr ("Contrabbassi", "Double basses");
        case Role::StringSection:  return tr ("Sezione archi", "String section");
        case Role::Flute:          return tr ("Flauto / ottavino", "Flute / piccolo");
        case Role::Oboe:           return tr ("Oboe / corno inglese", "Oboe / cor anglais");
        case Role::Clarinet:       return tr ("Clarinetto", "Clarinet");
        case Role::Bassoon:        return tr ("Fagotto", "Bassoon");
        case Role::WoodwindSection:return tr ("Sezione legni", "Woodwind section");
        case Role::FrenchHorns:    return tr ("Corni", "French horns");
        case Role::Trumpets:       return tr ("Trombe", "Trumpets");
        case Role::Trombones:      return tr ("Tromboni", "Trombones");
        case Role::Tuba:           return tr ("Tuba", "Tuba");
        case Role::BrassSection:   return tr ("Sezione ottoni", "Brass section");
        case Role::Timpani:        return tr ("Timpani", "Timpani");
        case Role::OrchPercussion: return tr ("Percussioni orchestrali (glockenspiel, xilofono, campane)", "Orchestral percussion (glockenspiel, xylophone, bells)");
        case Role::Harp:           return tr ("Arpa", "Harp");
        case Role::Choir:          return tr ("Coro", "Choir");
        case Role::PadWarm:        return tr ("Pad caldo (analogico)", "Warm pad (analog)");
        case Role::PadStrings:     return tr ("Pad archi", "String pad");
        case Role::PadChoir:       return tr ("Pad corale / voci", "Choir / vox pad");
        case Role::PadAmbient:     return tr ("Pad ambient / atmosfera", "Ambient / texture pad");
        case Role::PadBright:      return tr ("Pad luminoso / shimmer", "Bright / shimmer pad");
        default:                   return {};
    }
}

juce::StringArray Advisor::roleChoices()
{
    juce::StringArray s;
    for (int i = 0; i < kNumRoles; ++i) s.add (roleName ((Role) i));
    return s;
}

void Advisor::fillRoleCombo (juce::ComboBox& box)
{
    // gli item restano nell'ordine dei ruoli (l'indice coincide con il parametro); le intestazioni non contano come item
    for (int i = 0; i < kNumRoles; ++i)
    {
        if (i == 1)                     box.addSectionHeading (tr ("Band", "Band"));
        if (i == kFirstOrchestraRole)   box.addSectionHeading (tr ("Orchestra", "Orchestra"));
        if (i == kFirstPadRole)         box.addSectionHeading ("Pad");
        box.addItem (roleName ((Role) i), i + 1);
    }
}

juce::String Advisor::bandName (int b)
{
    static const char* it[] = { "Sub (20-60 Hz)", "Bassi (60-120 Hz)", "Calore (120-250 Hz)", "Medio-bassi (250-500 Hz)",
                                "Medi (0,5-2 kHz)", "Medio-alti (2-4 kHz)", "Presenza (4-8 kHz)", "Aria (8-20 kHz)" };
    static const char* en[] = { "Sub (20-60 Hz)", "Lows (60-120 Hz)", "Warmth (120-250 Hz)", "Low mids (250-500 Hz)",
                                "Mids (0.5-2 kHz)", "Upper mids (2-4 kHz)", "Presence (4-8 kHz)", "Air (8-20 kHz)" };
    b = juce::jlimit (0, kNumBands - 1, b);
    return tr (it[b], en[b]);
}

juce::String Advisor::freqText (float hz)
{
    if (hz < 1000.0f) return juce::String ((int) std::round (hz)) + " Hz";
    const float k = hz / 1000.0f;
    return num (k, std::abs (k - std::round (k)) < 0.05f ? 0 : 1) + " kHz";
}

juce::String Advisor::fullnessLabel (float f)
{
    if (f < 40) return tr ("Sottile", "Thin");
    if (f < 65) return tr ("Media", "Medium");
    if (f < 85) return tr ("Piena", "Full");
    return tr ("Densa", "Dense");
}

juce::StringArray Advisor::genreChoices()
{
    juce::StringArray s;
    for (int i = 0; i < kNumGenres; ++i) s.add (tr (genreProfile (i).nameIt, genreProfile (i).nameEn));
    return s;
}

juce::StringArray Advisor::targetChoices()
{
    return { tr ("Loudness tipica del genere", "Typical loudness for the genre"),
             tr ("Streaming (-14 LUFS)", "Streaming (-14 LUFS)"),
             tr ("Massimo impatto (-8 LUFS)", "Maximum impact (-8 LUFS)"),
             tr ("Mix per il mastering (picchi a -6 dBFS)", "Mix for mastering (peaks at -6 dBFS)") };
}

//==============================================================================
// Misure
void Advisor::relativeBands (const float* bands, float* rel)
{
    float mx = kSilenceDb;
    for (int b = 0; b < kNumBands; ++b) mx = juce::jmax (mx, bands[b]);
    for (int b = 0; b < kNumBands; ++b) rel[b] = bands[b] - mx;
}

void Advisor::deviation (const float* bands, const float* target, float* dev, float coreThreshold)
{
    float rel[kNumBands];
    relativeBands (bands, rel);
    double sum = 0; int n = 0;
    for (int b = 0; b < kNumBands; ++b)
    {
        dev[b] = rel[b] - target[b];
        if (target[b] >= coreThreshold) { sum += dev[b]; ++n; }
    }
    const float offset = n > 0 ? (float) (sum / n) : 0.0f;
    for (int b = 0; b < kNumBands; ++b) dev[b] -= offset;
}

float Advisor::trackFullness (const float* bands, float crest, Role role)
{
    const auto& prof = profileFor (role);
    float rel[kNumBands];
    relativeBands (bands, rel);
    int core = 0, covered = 0;
    for (int b = 0; b < kNumBands; ++b)
        if (prof.bands[b] >= -12.0f)
        {
            ++core;
            if (rel[b] >= prof.bands[b] - 8.0f) ++covered;
        }
    const float coverage = core > 0 ? (float) covered / (float) core : 0.0f;
    const float density = juce::jlimit (0.0f, 1.0f, (20.0f - crest) / 14.0f);
    return 100.0f * (0.65f * coverage + 0.35f * density);
}

float Advisor::mixFullness (const MixInfo& m)
{
    float dev[kNumBands];
    deviation (m.bandsLong, m.targetCurve(), dev, -20.0f);
    int ok = 0;
    for (int b = 0; b < kNumBands; ++b) if (std::abs (dev[b]) <= 6.0f) ++ok;
    const float coverage = (float) ok / (float) kNumBands;
    const float density = juce::jlimit (0.0f, 1.0f, (18.0f - m.crestDb) / 12.0f);
    return 100.0f * (0.6f * coverage + 0.4f * density);
}

int Snapshot::indexOfSlot (int slot) const
{
    for (int i = 0; i < tracks.size(); ++i)
        if (tracks.getReference (i).slot == slot) return i;
    return -1;
}

//==============================================================================
Snapshot Advisor::capture()
{
    Snapshot snap;
    auto& bus = SharedBus::get().bus();
    const auto& h = bus.header;

    auto& m = snap.mix;
    snap.connected = SharedBus::get().isInterProcess();
    // tutti i valori letti dalla memoria condivisa vengono ripuliti (mai NaN/Inf verso la grafica)
    auto db = [] (float v) { return juce::jlimit (kSilenceDb, 40.0f, finiteOr (v, kSilenceDb)); };
    auto unit = [] (float v) { return juce::jlimit (-1.0f, 1.0f, finiteOr (v, 1.0f)); };
    m.masterPresent = SharedBus::isFresh (h.masterLastSeen);
    m.shortLufs = db (h.masterShort);
    m.integratedLufs = db (h.masterIntegrated);
    m.recentPeakDb = db (h.masterRecentPeak);
    m.maxPeakDb = db (h.masterMaxPeak);
    m.crestDb = juce::jlimit (0.0f, 60.0f, finiteOr (h.masterCrest, 0.0f));
    m.correlation = unit (h.masterCorrelation);
    m.lowCorrelation = unit (h.masterLowCorrelation);
    m.activeSeconds = juce::jlimit (0.0f, 1.0e7f, finiteOr (h.masterActiveSeconds, 0.0f));
    m.genre = juce::jlimit (0, kNumGenres - 1, (int) h.genre);
    m.loudTarget = juce::jlimit (0, kNumTargets - 1, (int) h.loudTarget);
    m.refValid = h.refValid != 0 && h.useReference != 0;
    m.refLufs = db (h.refLufs);
    for (int b = 0; b < kNumBands; ++b)
    {
        m.bandsLong[b] = db (h.masterBandsLong[b]);
        m.bandsShort[b] = db (h.masterBandsShort[b]);
        m.refBands[b] = db (h.refBands[b]);
    }
    if (m.refValid)
    {
        float rel[kNumBands];
        relativeBands (m.refBands, rel);
        std::copy (rel, rel + kNumBands, m.refBands);
    }
    m.fullness = m.analysed() ? mixFullness (m) : 0.0f;

    for (int i = 0; i < kMaxSlots; ++i)
    {
        const auto& s = bus.slots[i];
        if (s.ownerId == 0 || ! SharedBus::isFresh (s.lastSeenMs)) continue;

        TrackInfo t;
        t.slot = i;
        t.name = juce::String::fromUTF8 (s.name, (int) strnlen (s.name, sizeof (s.name)));
        if (t.name.isEmpty()) t.name = tr ("Traccia ", "Track ") + juce::String (i + 1);
        t.role = (Role) juce::jlimit (1, kNumRoles - 1, (int) s.role);
        t.colour = juce::Colour (s.colour);
        t.numChannels = juce::jlimit (1, 2, (int) s.numChannels);
        t.excluded = s.excluded != 0;
        for (int k = 0; k < fsCount; ++k) t.fixState[k] = finiteOr (s.fixState[k], 0.0f);
        t.trimDb = juce::jlimit (-24.0f, 24.0f, finiteOr (s.trimDb, 0.0f));
        auto add = [&t, &db] (float v) { v = db (v); return v > -99.0f ? v + t.trimDb : v; };
        t.shortLufs = add (s.shortLufs);
        t.integratedLufs = add (s.integratedLufs);
        t.rawIntegrated = db (s.integratedLufs);
        t.recentPeakDb = add (s.recentPeakDb);
        t.maxPeakDb = add (s.maxPeakDb);
        t.crestDb = juce::jlimit (0.0f, 60.0f, finiteOr (s.crestDb, 0.0f));
        t.correlation = unit (s.correlation);
        t.lowCorrelation = unit (s.lowCorrelation);
        t.activeSeconds = juce::jlimit (0.0f, 1.0e7f, finiteOr (s.activeSeconds, 0.0f));
        for (int b = 0; b < kNumBands; ++b)
        {
            t.bandsLong[b] = add (s.bandsLong[b]);
            t.bandsShort[b] = add (s.bandsShort[b]);
        }
        snap.tracks.add (t);
    }

    // somma in potenza delle tracce analizzate (bus/gruppi esclusi)
    double sum = 0;
    for (auto& t : snap.tracks)
        if (t.analysed() && t.role != Role::Group) sum += dbToPow (t.integratedLufs);
    m.tracksSumLufs = powToDb (sum);

    const auto& gp = genreProfile (m.genre);
    for (auto& t : snap.tracks)
    {
        if (! t.analysed()) continue;
        t.fullness = trackFullness (t.bandsLong, t.crestDb, t.role);
        if (t.role == Role::Group) continue;
        t.relLevel = t.integratedLufs - m.tracksSumLufs;

        const auto& prof = profileFor (t.role);
        if (std::isnan (prof.level)) continue;
        int same = 0;
        for (auto& o : snap.tracks) if (o.analysed() && o.role == t.role) ++same;
        const int fam = roleFamily (t.role);
        t.targetRel = prof.level + (fam >= 0 ? gp.offset[fam] : 0.0f) - 10.0f * std::log10 ((float) juce::jmax (1, same));
    }

    // Normalizzazione: i livelli relativi delle tracce sommano sempre a 0 dB (la loro somma e' il mix).
    // Anche i riferimenti devono sommare a 0 dB sulle tracce presenti, altrimenti sarebbe impossibile
    // accontentarle tutte: abbassando una traccia, le altre risulterebbero automaticamente "piu' alte".
    {
        double targetSum = 0, otherShare = 0;
        int targeted = 0;
        for (auto& t : snap.tracks)
        {
            if (! t.analysed() || t.role == Role::Group || std::isnan (t.relLevel)) continue;
            if (std::isnan (t.targetRel)) otherShare += dbToPow (t.relLevel);
            else { targetSum += dbToPow (t.targetRel); ++targeted; }
        }
        snap.balanceReady = targeted >= 3;
        if (targeted >= 2 && targetSum > 0)
        {
            const double avail = juce::jmax (0.1, 1.0 - otherShare);
            const float offset = (float) (10.0 * std::log10 (avail / targetSum));
            juce::Array<float> deltas;
            for (auto& t : snap.tracks)
                if (! std::isnan (t.targetRel) && ! std::isnan (t.relLevel))
                {
                    t.targetRel += offset;
                    deltas.add (t.targetRel - t.relLevel);
                }

            // le correzioni partono dalla traccia "mediana": i Trim restano vicini a 0 dB
            std::sort (deltas.begin(), deltas.end());
            const float median = deltas.isEmpty() ? 0.0f : deltas[deltas.size() / 2];
            for (auto& t : snap.tracks)
                if (! std::isnan (t.targetRel) && ! std::isnan (t.relLevel))
                    t.levelDelta = (t.targetRel - t.relLevel) - median;
        }
    }
    snap.conflictList = conflicts (snap);
    return snap;
}

//==============================================================================
juce::Array<Conflict> Advisor::conflicts (const Snapshot& s)
{
    static const float weight[kNumBands] = { 0.8f, 1.0f, 1.0f, 1.0f, 0.7f, 1.0f, 0.8f, 0.5f };
    juce::Array<Conflict> out;

    for (int i = 0; i < s.tracks.size(); ++i)
    {
        const auto& a = s.tracks.getReference (i);
        if (! a.analysed() || a.role == Role::Group) continue;
        float ra[kNumBands]; relativeBands (a.bandsLong, ra);

        for (int j = i + 1; j < s.tracks.size(); ++j)
        {
            const auto& b = s.tracks.getReference (j);
            if (! b.analysed() || b.role == Role::Group) continue;
            float rb[kNumBands]; relativeBands (b.bandsLong, rb);

            Conflict best { i, j, 0, 0.0f };
            for (int k = 0; k < kNumBands; ++k)
            {
                const float sigA = juce::jlimit (0.0f, 1.0f, (ra[k] + 6.0f) / 6.0f);
                const float sigB = juce::jlimit (0.0f, 1.0f, (rb[k] + 6.0f) / 6.0f);
                const float diff = std::abs (a.bandsLong[k] - b.bandsLong[k]);
                const float score = weight[k] * juce::jmax (0.0f, 1.0f - diff / 6.0f) * juce::jmin (sigA, sigB);
                if (score > best.score) { best.score = score; best.band = k; }
            }
            if (best.score >= 0.35f) out.add (best);
        }
    }
    std::sort (out.begin(), out.end(), [] (const Conflict& x, const Conflict& y) { return x.score > y.score; });
    return out;
}

//==============================================================================
static void sortAdvice (juce::Array<Advice>& a)
{
    std::stable_sort (a.begin(), a.end(), [] (const Advice& x, const Advice& y) { return x.severity > y.severity; });
}

static juce::String eqWhy (int band, bool cut)
{
    static const char* cutIt[] = { "rombo e sub inutili", "bassi gonfi", "pesantezza e rimbombo", "suono scatolato e fangoso",
                                   "suono nasale, da telefono", "asprezza che affatica l'ascolto", "sibilanti e spigoli", "fruscio e frizzantezza eccessiva" };
    static const char* cutEn[] = { "unneeded rumble and sub", "boomy lows", "heaviness and boom", "boxy, muddy tone",
                                   "nasal, honky tone", "harshness that tires the ear", "sibilance and edginess", "hiss and excessive fizz" };
    static const char* boostIt[] = { "peso nella parte bassa", "fondamentale e spinta", "corpo e calore", "pienezza",
                                     "presenza nel mix", "intelligibilita' e attacco", "definizione", "aria e brillantezza" };
    static const char* boostEn[] = { "low-end weight", "fundamental and punch", "body and warmth", "fullness",
                                     "presence in the mix", "clarity and attack", "definition", "air and sparkle" };
    return cut ? tr (cutIt[band], cutEn[band]) : tr (boostIt[band], boostEn[band]);
}

static float roundHalf (float v) { return std::round (v * 2.0f) / 2.0f; }

juce::Array<Advice> Advisor::forTrack (const Snapshot& s, int idx)
{
    juce::Array<Advice> out;
    if (idx < 0 || idx >= s.tracks.size()) return out;
    const auto& t = s.tracks.getReference (idx);
    const auto& prof = profileFor (t.role);
    const auto& gp = genreProfile (s.mix.genre);
    const auto& cat = PluginCatalog::get();

    auto add = [&out, &t] (const juce::String& key, int sev, const juce::String& category, const juce::String& title, const juce::String& detail,
                       const juce::String& plugin = {}, float delta = mpm::kNaN)
    {
        Advice a;
        a.key = key; a.severity = sev; a.category = category; a.title = title; a.detail = detail; a.plugin = plugin; a.levelDelta = delta;
        if (std::isfinite (delta)) a.levelTarget = juce::jlimit (-24.0f, 24.0f, t.trimDb + delta);
        out.add (a);
    };

    const auto catLevel = tr ("Livello", "Level");
    const auto catEq = "EQ";
    const auto catDyn = tr ("Dinamica", "Dynamics");
    const auto catSpace = tr ("Spazio", "Space");
    const auto catStereo = "Stereo";
    const auto catGain = tr ("Guadagno", "Gain");

    if (t.excluded)
    {
        add ("_excluded", 0, tr ("Analisi", "Analysis"), tr ("Traccia esclusa dall'analisi", "Track excluded from analysis"),
             tr ("La traccia e' in mute o in bypass e non viene considerata nel mix. Riattivala con l'interruttore Analizza.",
                 "The track is muted or bypassed and is not considered in the mix. Turn it back on with the Analyse switch."));
        return out;
    }

    if (! t.analysed())
    {
        const float remaining = juce::jmax (1.0f, 8.0f - t.activeSeconds);
        add ("_analysing", 0, tr ("Analisi", "Analysis"), tr ("Analisi in corso", "Analysing"),
             tr ("Fai suonare il brano: servono ancora circa ", "Play the song: about ")
                 + juce::String ((int) std::ceil (remaining))
                 + tr (" secondi di musica su questa traccia.", " more seconds of music are needed on this track."));
        return out;
    }

    // Guadagno / picchi
    if (t.maxPeakDb - t.trimDb > -0.5f)
        add ("peak", 1, catGain, tr ("Picchi vicini allo 0 dBFS", "Peaks close to 0 dBFS"),
             tr ("Picco massimo in ingresso ", "Max input peak ") + num (t.maxPeakDb - t.trimDb) + tr (" dBFS. Abbassa il Clip Gain dell'evento audio (non il Trim) di circa ",
                                                                         " dBFS. Lower the audio event Clip Gain (not the Trim) by about ")
                 + num (t.maxPeakDb - t.trimDb + 6.0f, 0) + tr (" dB: i plug-in prima di May Pro Mix lavorano meglio con picchi intorno a -6 dBFS. Il Trim serve solo al bilanciamento.",
                                                       " dB: plug-ins before May Pro Mix work better with peaks around -6 dBFS. The Trim is only for balance."));
    else if (t.integratedLufs < -38.0f && t.maxPeakDb < -20.0f)
        add ("lowsig", 0, catGain, tr ("Segnale molto basso", "Very low signal"),
             tr ("Alza il Clip Gain di qualche dB: i plug-in di emulazione analogica rendono meglio con livelli intorno a -18 dBFS RMS.",
                 "Raise Clip Gain a few dB: analogue-style plug-ins work best around -18 dBFS RMS."));

    if (t.role == Role::Group)
    {
        add ("_group", 0, tr ("Ruolo", "Role"), tr ("Bus escluso dal bilanciamento", "Bus excluded from balance"),
             tr ("Questa traccia e' un bus: non viene sommata alle altre per evitare di contarla due volte.",
                 "This track is a bus: it is not summed with the others so it isn't counted twice."));
        sortAdvice (out);
        return out;
    }

    if (t.role == Role::Other)
        add ("_role", 0, tr ("Ruolo", "Role"), tr ("Scegli il ruolo della traccia", "Choose the track role"),
             tr ("Con un ruolo preciso (voce, basso, chitarra...) i consigli su livello, EQ e dinamica diventano molto piu' accurati.",
                 "With a precise role (vocal, bass, guitar...) level, EQ and dynamics advice become much more accurate."));

    // Livello nel mix (solo con almeno 3 tracce analizzate: prima il confronto non e' affidabile)
    if (s.balanceReady && std::isfinite (t.levelDelta))
    {
        const float delta = roundHalf (juce::jlimit (-24.0f, 24.0f, t.levelDelta));
        if (std::abs (delta) >= 2.0f)
        {
            const bool up = delta > 0;
            add ("lvl", std::abs (delta) >= 6.0f ? 2 : 1, catLevel,
                 (up ? tr ("Alza di ", "Raise by ") : tr ("Abbassa di ", "Lower by ")) + num (std::abs (delta)) + " dB",
                 tr ("La traccia e' a ", "The track sits at ") + num (t.relLevel, 1, true) + tr (" LU rispetto al mix; per ", " LU relative to the mix; for ")
                     + tr (gp.nameIt, gp.nameEn) + tr (", con le tracce di questo brano, il riferimento e' circa ", ", with the tracks of this song, the reference is about ") + num (t.targetRel, 1, true)
                     + tr (" LU. La correzione tiene conto anche delle altre tracce: con \"Bilancia tutte\" nel Master le sistemi in un colpo solo.",
                           " LU. The correction also accounts for the other tracks: \"Balance all\" in the Master fixes them in one go."),
                 {}, delta);
        }
    }

    // EQ: passa-alto
    float rel[kNumBands]; relativeBands (t.bandsLong, rel);
    bool hpfDone = false;
    if (prof.hpfHz > 0)
    {
        const bool lowRole = t.role == Role::Kick || t.role == Role::Bass || t.role == Role::DrumBus
                          || t.role == Role::DoubleBasses || t.role == Role::Tuba;
        const bool excess = lowRole ? (rel[0] > rel[1] + 3.0f)
                                    : (rel[0] > prof.bands[0] + 8.0f || (prof.hpfHz >= 100 && rel[1] > prof.bands[1] + 8.0f));
        if (excess)
        {
            hpfDone = true;
            add ("hpf", 1, catEq, tr ("Filtro passa-alto a ", "High-pass filter at ") + freqText (prof.hpfHz),
                 tr ("Energia inutile sotto ", "Unneeded energy below ") + freqText (prof.hpfHz)
                     + tr (": toglila con un passa-alto da 12-24 dB/ottava. Libera spazio per cassa e basso e pulisce il mix.",
                           ": remove it with a 12-24 dB/oct high-pass. It frees room for kick and bass and cleans up the mix."),
                 cat.suggest (Tool::EQ));
            auto& last = out.getReference (out.size() - 1);
            last.band = rel[0] > prof.bands[0] + 8.0f ? 0 : 1;
            last.fix.type = FixType::Hpf;
            last.fix.freq = prof.hpfHz;
            last.fix.slope = 12;
        }
    }

    // EQ: scostamenti dal profilo del ruolo
    if (t.role != Role::Other)
    {
        float dev[kNumBands];
        deviation (t.bandsLong, prof.bands, dev);
        struct Move { int band; float amount; };
        juce::Array<Move> moves;
        for (int b = 0; b < kNumBands; ++b)
        {
            if (prof.bands[b] < -20.0f) continue;
            if (hpfDone && b <= 1) continue;
            if (dev[b] > 4.0f) moves.add ({ b, -juce::jlimit (1.5f, 6.0f, roundHalf (dev[b] * 0.6f)) });
            else if (dev[b] < -5.0f && dev[b] > -18.0f && prof.bands[b] >= -10.0f)
                moves.add ({ b, juce::jlimit (1.0f, 4.0f, roundHalf (-dev[b] * 0.5f)) });
        }
        std::sort (moves.begin(), moves.end(), [] (const Move& x, const Move& y) { return std::abs (x.amount) > std::abs (y.amount); });

        for (int i = 0; i < juce::jmin (3, moves.size()); ++i)
        {
            const auto mv = moves[i];
            const bool cut = mv.amount < 0;
            const auto shape = mv.band == kNumBands - 1 ? tr ("Shelf alto da 10 kHz", "High shelf from 10 kHz")
                             : mv.band == 4 ? tr ("Campana larga, Q 0,7", "Wide bell, Q 0.7")
                                            : tr ("Campana, Q ", "Bell, Q ") + num (cut ? 1.4f : 1.0f);
            const bool sibilant = cut && mv.band == 6 && (t.role == Role::LeadVocal || t.role == Role::BackingVocal || t.role == Role::Choir);
            add ("eq" + juce::String (mv.band), std::abs (mv.amount) >= 5.0f ? 2 : 1, catEq,
                 (cut ? tr ("Taglia ", "Cut ") : tr ("Aggiungi ", "Boost ")) + num (std::abs (mv.amount)) + " dB "
                     + tr ("a ", "at ") + freqText (kBandCentre[mv.band]),
                 bandName (mv.band) + ": " + (cut ? tr ("troppa energia, ", "too much energy, ") : tr ("manca un po' di ", "lacking some "))
                     + eqWhy (mv.band, cut) + ". " + shape + "."
                     + (sibilant ? tr (" Sulla voce preferisci un de-esser tra 5 e 8 kHz.", " On vocals prefer a de-esser between 5 and 8 kHz.") : juce::String()),
                 sibilant ? cat.suggest (Tool::DeEsser) : cat.suggest (Tool::EQ));
            auto& last = out.getReference (out.size() - 1);
            last.band = mv.band;
            if (! sibilant)   // sulle sibilanti serve un de-esser: nessun preset Pro EQ3
            {
                last.fix.type = FixType::Eq;
                last.fix.gain = mv.amount;
                if (mv.band == kNumBands - 1) { last.fix.shape = 2; last.fix.freq = 10000.0f; last.fix.q = 0.7f; }
                else { last.fix.shape = 0; last.fix.freq = kBandCentre[mv.band]; last.fix.q = mv.band == 4 ? 0.7f : (cut ? 1.4f : 1.0f); }
            }
        }
    }

    // Dinamica (corretta dal genere)
    if (t.role != Role::Other)
    {
        const float cMin = prof.crestMin + gp.crestShift, cMax = prof.crestMax + gp.crestShift;
        const float centre = 0.5f * (cMin + cMax);
        if (t.crestDb > cMax + 1.0f)
        {
            const float gr = juce::jlimit (2.0f, 8.0f, roundHalf (t.crestDb - centre));
            auto detail = tr ("Fattore di cresta ", "Crest factor ") + num (t.crestDb) + tr (" dB (per questo ruolo nel ", " dB (for this role in ")
                          + tr (gp.nameIt, gp.nameEn) + tr (": ", ": ") + num (cMin, 0) + "-" + num (cMax, 0)
                          + tr (" dB). Ratio ", " dB). Ratio ") + prof.ratio + tr (", attacco ", ", attack ") + juce::String (prof.attackMs)
                          + tr (" ms, rilascio ", " ms, release ") + juce::String (prof.releaseMs)
                          + tr (" ms, soglia regolata per circa ", " ms, threshold set for about ") + num (gr, 1) + tr (" dB di riduzione sui passaggi forti.", " dB of gain reduction on loud passages.");
            if (t.role == Role::LeadVocal && gr >= 5.0f)
                detail << tr (" Meglio in due stadi: un compressore veloce sui picchi e uno lento (stile opto) per livellare.",
                              " Better in two stages: a fast compressor for peaks and a slow (opto style) one to level.");
            add ("comp", gr >= 6.0f ? 2 : 1, catDyn, tr ("Comprimi: circa ", "Compress: about ") + num (gr) + tr (" dB di riduzione", " dB of gain reduction"),
                 detail, cat.suggest (Tool::Compressor));
            {
                // soglia calcolata sul livello misurato (dopo i plug-in in catena, prima del trim)
                auto& last = out.getReference (out.size() - 1);
                const float ratio = juce::jmax (1.5f, juce::String (prof.ratio).upToFirstOccurrenceOf (":", false, false).getFloatValue());
                const float slope = 1.0f - 1.0f / ratio;
                const bool ours = t.fixState[fsEnabled] > 0.5f && t.fixState[fsCompOn] > 0.5f;
                last.fix.type = FixType::Comp;
                last.fix.ratio = ratio;
                last.fix.attack = (float) prof.attackMs;
                last.fix.release = (float) prof.releaseMs;
                last.fix.thr = ours ? t.fixState[fsCompThr] - gr * 0.6f / slope
                                    : (t.rawIntegrated + 3.0f) - gr / slope;
                last.fix.thr = juce::jlimit (-60.0f, -1.0f, std::round (last.fix.thr));
                last.fix.makeup = ours ? juce::jlimit (0.0f, 18.0f, t.fixState[fsCompMakeup] + std::round (gr * 0.3f))
                                       : std::round (gr * 0.5f);
            }
        }
        else if (t.crestDb < cMin - 1.5f && t.crestDb > 0.5f)
        {
            add ("squash", 1, catDyn, tr ("Dinamica schiacciata", "Squashed dynamics"),
                 tr ("Fattore di cresta ", "Crest factor ") + num (t.crestDb) + tr (" dB, sotto il minimo consigliato di ", " dB, below the suggested minimum of ")
                     + num (cMin, 0) + tr (" dB: alza la soglia o riduci il ratio. Per ritrovare i transienti prova un attacco piu' lento (30 ms).",
                                           " dB: raise the threshold or lower the ratio. To bring transients back try a slower attack (30 ms)."),
                 cat.suggest (t.role == Role::Kick || t.role == Role::Snare ? Tool::Transient : Tool::Compressor));
            if (t.fixState[fsEnabled] > 0.5f && t.fixState[fsCompOn] > 0.5f)
                out.getReference (out.size() - 1).fix.type = FixType::CompSoften;
        }
    }

    // Stereo
    if (t.numChannels >= 2)
    {
        if (t.correlation < 0.0f)
            add ("phase", 2, catStereo, tr ("Problema di fase", "Phase problem"),
                 tr ("Correlazione ", "Correlation ") + num (t.correlation, 2)
                     + tr (": in mono la traccia si svuota. Controlla la polarita' dei microfoni o allineali nel tempo.",
                           ": the track collapses in mono. Check microphone polarity or time-align them."));
        else if ((t.role == Role::Kick || t.role == Role::Bass || t.role == Role::DoubleBasses || t.role == Role::Tuba) && t.lowCorrelation < 0.8f)
            add ("lowmono", 1, catStereo, tr ("Bassi non mono", "Low end not mono"),
                 tr ("Sotto i 120 Hz la traccia e' larga: rendila mono in quella zona per un mix piu' solido e compatibile.",
                     "Below 120 Hz the track is wide: make it mono there for a solid, compatible mix."),
                 cat.suggest (Tool::Imager));
        else if (t.correlation > 0.97f && (t.role == Role::RhythmGuitar || t.role == Role::Keys || t.role == Role::Pad || t.role == Role::AcousticGuitar
                                           || t.role == Role::StringSection || t.role == Role::Choir || (int) t.role >= kFirstPadRole))
            add ("fakestereo", 0, catStereo, tr ("Stereo solo di nome", "Stereo in name only"),
                 tr ("La traccia e' praticamente mono: puoi convertirla in mono e pannarla, oppure registrare una seconda ripresa da aprire a sinistra e destra.",
                     "The track is practically mono: convert it to mono and pan it, or record a second take to spread left and right."));
    }

    // Collisioni con altre tracce
    int shown = 0;
    for (auto& c : s.conflictList)
    {
        if (shown >= 2) break;
        if (c.a != idx && c.b != idx) continue;
        const int otherIdx = c.a == idx ? c.b : c.a;
        const auto& o = s.tracks.getReference (otherIdx);
        const auto& oprof = profileFor (o.role);
        const bool iYield = prof.priority < oprof.priority || (prof.priority == oprof.priority && idx > otherIdx);
        const auto band = bandName (c.band);
        const auto f = freqText (kBandCentre[c.band]);
        ++shown;

        const bool kickBass = (t.role == Role::Kick && o.role == Role::Bass) || (t.role == Role::Bass && o.role == Role::Kick);
        const bool vsVocal = o.role == Role::LeadVocal && c.band >= 3 && c.band <= 6;
        const bool guitars = roleFamily (t.role) >= 3 && roleFamily (t.role) <= 4 && roleFamily (o.role) >= 3 && roleFamily (o.role) <= 4;

        if (! iYield)
        {
            add ("_yield" + juce::String (o.slot), 0, catSpace, tr ("Si sovrappone a ", "Overlaps with ") + o.name,
                 band + tr (": conviene intervenire su ", ": better to act on ") + o.name + tr (" per lasciare spazio a questa traccia.", " to leave room for this track."));
            continue;
        }

        const int before = out.size();
        if (kickBass && c.band <= 2)
            add ("mask" + juce::String (o.slot), c.score > 0.8f ? 2 : 1, catSpace, tr ("Cassa e basso si pestano i piedi", "Kick and bass are fighting"),
                 tr ("Entrambi forti in ", "Both strong in ") + band
                     + tr (". Sul basso: compressore in sidechain dalla cassa (ratio 4:1, attacco 1 ms, rilascio 80 ms, 2-3 dB) e scegli chi domina: cassa a 60 Hz e basso a 100 Hz, o viceversa.",
                           ". On the bass: compressor sidechained from the kick (4:1, 1 ms attack, 80 ms release, 2-3 dB) and pick who owns what: kick at 60 Hz and bass at 100 Hz, or the other way round."),
                 cat.suggest (Tool::Compressor));
        else if (vsVocal)
            add ("mask" + juce::String (o.slot), c.score > 0.8f ? 2 : 1, catSpace, tr ("Copre la voce in ", "Masks the vocal in ") + f,
                 tr ("Taglia 2-3 dB attorno a ", "Cut 2-3 dB around ") + f
                     + tr (" su questa traccia, meglio con una banda dinamica in sidechain dalla voce: si abbassa solo quando la voce canta.",
                           " on this track, ideally with a dynamic band sidechained from the vocal so it only dips while the vocal sings."),
                 cat.suggest (Tool::DynamicEQ));
        else if (guitars)
            add ("mask" + juce::String (o.slot), 1, catSpace, tr ("Si sovrappone a ", "Overlaps with ") + o.name,
                 tr ("Due chitarre nella stessa zona (", "Two guitars in the same zone (") + band
                     + tr ("). Aprile in stereo (per esempio L70 e R70) e scava 2 dB a ", "). Spread them (e.g. L70 and R70) and carve 2 dB at ")
                     + f + tr (" su questa.", " on this one."),
                 cat.suggest (Tool::EQ));
        else
            add ("mask" + juce::String (o.slot), c.score > 0.8f ? 2 : 1, catSpace, tr ("Si sovrappone a ", "Overlaps with ") + o.name,
                 band + tr (": taglia 2-3 dB a ", ": cut 2-3 dB at ") + f + tr (" (Q 1,5) su questa traccia per lasciare spazio a ", " (Q 1.5) on this track to make room for ") + o.name + ".",
                 cat.suggest (Tool::EQ));

        // con un clic: taglio a campana sulla traccia meno importante (non per cassa/basso: li' serve il sidechain)
        if (out.size() > before && ! (kickBass && c.band <= 2))
        {
            auto& last = out.getReference (out.size() - 1);
            last.fix.type = FixType::Eq;
            last.fix.shape = 0;
            last.fix.freq = kBandCentre[c.band];
            last.fix.gain = guitars ? -2.0f : -2.5f;
            last.fix.q = vsVocal ? 1.2f : 1.5f;
        }
    }

    if (out.isEmpty())
        add ("_ok", 0, "OK", tr ("Traccia in equilibrio", "Track is balanced"),
             tr ("Livello, timbro e dinamica sono in linea con il ruolo e il genere scelti.",
                 "Level, tone and dynamics match the chosen role and genre."));

    sortAdvice (out);
    return out;
}

//==============================================================================
juce::Array<Advice> Advisor::forMix (const Snapshot& s)
{
    juce::Array<Advice> out;
    const auto& m = s.mix;
    const auto& gp = genreProfile (m.genre);
    const auto& cat = PluginCatalog::get();
    auto add = [&out] (const juce::String& key, int sev, const juce::String& category, const juce::String& title, const juce::String& detail, const juce::String& plugin = {})
    {
        Advice a;
        a.key = key; a.severity = sev; a.category = category; a.title = title; a.detail = detail; a.plugin = plugin;
        out.add (a);
    };
    const auto catMaster = "Master";
    const auto catTone = tr ("Timbro", "Tone");

    if (! s.connected)
        add ("_nobus", 2, tr ("Collegamento", "Connection"), tr ("Collegamento tra i plug-in non disponibile", "Plug-ins cannot talk to each other"),
             tr ("Il sistema non permette la memoria condivisa: Track e Master non possono scambiarsi i dati. Riavvia Studio Pro; se il problema resta, segnalalo.",
                 "The system does not allow shared memory: Track and Master cannot exchange data. Restart Studio Pro; if it persists, please report it."));

    if (s.tracks.isEmpty())
        add ("_notracks", 1, tr ("Inizio", "Start"), tr ("Nessuna traccia collegata", "No tracks connected"),
             tr ("Inserisci May Pro Mix Track come ultimo insert su ogni traccia e scegli il ruolo (o lascia Automatico e dai nomi chiari alle tracce).",
                 "Insert May Pro Mix Track as the last insert on every track and choose the role (or leave Automatic and name your tracks clearly)."));

    if (! m.masterPresent)
        add ("_nomaster", 1, tr ("Inizio", "Start"), tr ("Master non trovato", "Master not found"),
             tr ("Inserisci May Pro Mix Master come ultimo plug-in sul canale Main.", "Insert May Pro Mix Master as the last plug-in on the Main channel."));
    else if (! m.analysed())
        add ("_analysing", 0, tr ("Analisi", "Analysis"), tr ("Analisi in corso", "Analysing"),
             tr ("Fai suonare il brano dall'inizio alla fine (o almeno strofa e ritornello). Poi premi Azzera dopo ogni modifica importante.",
                 "Play the song from start to finish (or at least verse and chorus). Press Reset after every major change."));

    add ("_genre", 0, tr ("Genere", "Genre"), tr (gp.nameIt, gp.nameEn), tr (gp.tipIt, gp.tipEn));

    if (m.analysed())
    {
        const float target = targetLufs (m.loudTarget, m.genre);
        if (m.loudTarget == targetPreMaster)
        {
            if (m.maxPeakDb > -3.0f)
                add ("premaster", 2, catMaster, tr ("Lascia spazio al mastering", "Leave headroom for mastering"),
                     tr ("Picchi a ", "Peaks at ") + num (m.maxPeakDb) + tr (" dBFS: abbassa il master (o tutte le tracce) per avere picchi intorno a -6 dBFS e togli il limiter dal bus.",
                                                                          " dBFS: lower the master (or all tracks) to peak around -6 dBFS and remove the limiter from the bus."));
        }
        else
        {
            const float delta = target - m.integratedLufs;
            if (std::abs (delta) > 1.5f)
            {
                auto detail = tr ("Loudness integrata ", "Integrated loudness ") + num (m.integratedLufs) + tr (" LUFS, obiettivo ", " LUFS, target ") + num (target, 0)
                              + " LUFS. ";
                if (delta > 0)
                    detail << tr ("Alza il guadagno del limiter di circa ", "Raise the limiter gain by about ") + num (delta, 1)
                           << tr (" dB con ceiling a -1 dB.", " dB with the ceiling at -1 dB.");
                else
                    detail << tr ("Riduci il guadagno del limiter di circa ", "Reduce the limiter gain by about ") + num (-delta, 1) + " dB.";
                if (delta > 6.0f)
                    detail << tr (" Sono molti dB: prima comprimi il bus (2:1, attacco 30 ms, 2 dB di riduzione), poi limita.",
                                  " That is a lot: compress the bus first (2:1, 30 ms attack, 2 dB reduction), then limit.");
                add ("loud", std::abs (delta) > 4.0f ? 2 : 1, catMaster,
                     tr ("Loudness: ", "Loudness: ") + num (delta, 1, true) + " dB", detail, cat.suggest (Tool::Limiter));
            }
            if (m.maxPeakDb > -1.0f)
                add ("mixpeak", 2, catMaster, tr ("Picchi oltre -1 dBFS", "Peaks above -1 dBFS"),
                     tr ("Imposta il ceiling del limiter a -1 dB: evita distorsioni quando lo streaming converte in MP3/AAC.",
                         "Set the limiter ceiling to -1 dB to avoid distortion when streaming converts to MP3/AAC."),
                     cat.suggest (Tool::Limiter));
        }

        // Bilanciamento tonale
        float dev[kNumBands];
        deviation (m.bandsLong, m.targetCurve(), dev, -20.0f);
        for (int b = 0; b < kNumBands; ++b)
        {
            if (std::abs (dev[b]) <= 3.0f) continue;
            const bool cut = dev[b] > 0;
            int culprit = -1; float best = kSilenceDb;
            for (int i = 0; i < s.tracks.size(); ++i)
            {
                const auto& t = s.tracks.getReference (i);
                if (t.analysed() && t.role != Role::Group && t.bandsLong[b] > best) { best = t.bandsLong[b]; culprit = i; }
            }
            auto detail = bandName (b) + ": " + num (dev[b], 1, true) + tr (" dB rispetto al profilo ", " dB versus the ")
                          + (m.genre >= 0 && m.refValid ? tr ("del riferimento", "reference profile") : tr (gp.nameIt, gp.nameEn) + tr ("", " profile")) + ". ";
            if (culprit >= 0)
                detail << (cut ? tr ("Chi contribuisce di piu': ", "Main contributor: ") : tr ("Puoi lavorare su: ", "You could work on: "))
                       << s.tracks.getReference (culprit).name << ". ";
            detail << tr ("Sul master al massimo 1-2 dB con campana larga; meglio correggere sulle tracce.",
                          "On the master no more than 1-2 dB with a wide bell; better to fix it on the tracks.");
            add ("tone" + juce::String (b), std::abs (dev[b]) > 6.0f ? 2 : 1, catTone,
                 (cut ? tr ("Troppo: ", "Too much: ") : tr ("Poco: ", "Too little: ")) + bandName (b), detail, cat.suggest (Tool::EQ));
            out.getReference (out.size() - 1).band = b;
        }

        if (m.correlation < 0.3f)
            add ("corr", 2, "Stereo", tr ("Compatibilita' mono a rischio", "Mono compatibility at risk"),
                 tr ("Correlazione del mix ", "Mix correlation ") + num (m.correlation, 2) + tr (": controlla allargatori stereo e fase delle riprese.", ": check stereo wideners and the phase of your recordings."),
                 cat.suggest (Tool::Imager));
        if (m.lowCorrelation < 0.85f)
            add ("lowcorr", 1, "Stereo", tr ("Bassi del mix non mono", "Mix low end not mono"),
                 tr ("Sotto i 120 Hz il mix e' largo: tieni cassa, basso e sub al centro.", "Below 120 Hz the mix is wide: keep kick, bass and sub in the centre."),
                 cat.suggest (Tool::Imager));

        const float crestShift = gp.crestShift;
        if (m.crestDb < 6.0f + crestShift * 0.5f && m.loudTarget != targetPreMaster)
            add ("mixsquash", 1, catMaster, tr ("Mix molto schiacciato", "Very squashed mix"),
                 tr ("Fattore di cresta ", "Crest factor ") + num (m.crestDb) + tr (" dB: riduci limiter o compressione sul bus, il brano perde impatto.", " dB: reduce limiting or bus compression, the song is losing punch."));
        else if (m.crestDb > 16.0f + crestShift && m.loudTarget != targetPreMaster)
            add ("mixdyn", 1, catMaster, tr ("Mix molto dinamico", "Very dynamic mix"),
                 tr ("Fattore di cresta ", "Crest factor ") + num (m.crestDb) + tr (" dB: un compressore sul bus (2:1, attacco 30 ms, rilascio auto, 1-2 dB) dara' coesione.",
                                                                                   " dB: a bus compressor (2:1, 30 ms attack, auto release, 1-2 dB) will glue things together."),
                 cat.suggest (Tool::Compressor));
    }

    // Riepilogo tracce
    int levelIssues = 0;
    for (int i = 0; i < s.tracks.size(); ++i)
        for (auto& a : forTrack (s, i))
            if (! std::isnan (a.levelDelta)) { ++levelIssues; break; }
    if (levelIssues > 0)
        add ("_balance", 1, tr ("Bilanciamento", "Balance"),
             juce::String (levelIssues) + tr (" tracce con livello da sistemare", " tracks need a level change"),
             tr ("Seleziona le tracce con il pallino giallo o rosso per vedere quanto alzare o abbassare.",
                 "Select tracks with a yellow or red dot to see how much to raise or lower."));

    const auto& cs = s.conflictList;
    for (int i = 0; i < juce::jmin (3, cs.size()); ++i)
    {
        const auto& c = cs.getReference (i);
        add ("_ov" + juce::String (i), c.score > 0.6f ? 1 : 0, tr ("Spazio", "Space"),
             s.tracks.getReference (c.a).name + " / " + s.tracks.getReference (c.b).name,
             tr ("Si sovrappongono in ", "They overlap in ") + bandName (c.band) + tr (". Dettagli nella traccia meno importante.", ". Details on the less important track."));
    }

    if (m.genre >= 0 && s.mix.genre >= 0 && SharedBus::get().bus().header.useReference && ! m.refValid)
        add ("_ref", 1, tr ("Riferimento", "Reference"), tr ("Nessun riferimento memorizzato", "No reference stored"),
             tr ("Fai suonare un brano di riferimento sul master e premi Memorizza riferimento.", "Play a reference song through the master and press Store reference."));

    sortAdvice (out);
    return out;
}
} // namespace mpm

//==============================================================================
namespace mpm
{
juce::String Advisor::followUp (const juce::String& k)
{
    if (k == "lvl")        return tr ("Controlla che May Pro Mix Track sia l'ultimo insert e che non ci sia un'automazione di volume prima. Se la traccia cambia molto tra strofa e ritornello, automatizza il Trim o il fader per sezione.",
                                      "Check that May Pro Mix Track is the last insert and that there is no volume automation before it. If the track changes a lot between verse and chorus, automate the Trim or fader per section.");
    if (k == "peak")       return tr ("Se i picchi restano alti, un plug-in prima li ricrea (saturatore, EQ con boost): abbassa il suo output, oppure usa un limiter leggero sulla traccia con ceiling a -3 dB.",
                                      "If peaks stay high, a plug-in before is recreating them (saturator, boosting EQ): lower its output, or use a gentle limiter on the track with the ceiling at -3 dB.");
    if (k == "lowsig")     return tr ("Alza il guadagno alla fonte (Clip Gain dell'evento), non solo l'uscita di un plug-in.",
                                      "Raise the gain at the source (event Clip Gain), not just a plug-in output.");
    if (k == "hpf")        return tr ("Alza il passa-alto di 20-30 Hz o usa una pendenza piu' ripida (24 dB/ottava). Controlla anche rimbombi della stanza con una campana stretta.",
                                      "Raise the high-pass by 20-30 Hz or use a steeper slope (24 dB/oct). Also check for room boom with a narrow bell.");
    if (k.startsWith ("eq")) return tr ("Non e' bastato: fai uno sweep con una campana stretta (+8 dB, Q 6) per trovare la frequenza esatta e correggi li'. Se il problema va e viene usa una banda dinamica; se manca proprio il contenuto, l'EQ non lo crea: prova una leggera saturazione.",
                                        "Not enough: sweep a narrow bell (+8 dB, Q 6) to find the exact frequency and correct there. If the problem comes and goes use a dynamic band; if the content is simply missing, EQ can't create it: try gentle saturation.");
    if (k == "comp")       return tr ("La dinamica e' ancora ampia: abbassa la soglia di 2-3 dB oppure usa due compressori in serie (uno veloce sui picchi, uno lento per livellare). Sulla voce aiuta molto un'automazione di volume prima del compressore.",
                                      "Dynamics are still wide: lower the threshold by 2-3 dB or use two compressors in series (a fast one for peaks, a slow one to level). On vocals, volume automation before the compressor helps a lot.");
    if (k == "squash")     return tr ("Togli un compressore oppure usa la compressione parallela: 50% di segnale compresso e 50% originale.",
                                      "Remove one compressor or use parallel compression: 50% compressed and 50% original signal.");
    if (k == "phase")      return tr ("Inverti la polarita' di un microfono (tasto polarita') e confronta; se non basta, sposta una traccia di pochi millisecondi finche' i bassi tornano pieni.",
                                      "Flip the polarity of one microphone and compare; if that is not enough, nudge one track by a few milliseconds until the low end is full again.");
    if (k == "lowmono")    return tr ("Usa un plug-in di immagine stereo con la funzione mono sotto una frequenza, oppure rendi mono l'intera traccia.",
                                      "Use a stereo imaging plug-in with a mono-below-frequency function, or make the whole track mono.");
    if (k == "fakestereo") return tr ("Converti la traccia in mono, oppure registra una seconda ripresa vera: un raddoppio digitale non da' la stessa larghezza.",
                                      "Convert the track to mono, or record a real second take: a digital double doesn't give the same width.");
    if (k.startsWith ("mask")) return tr ("La sovrapposizione resta: prova un EQ dinamico in sidechain, sposta uno dei due strumenti di ottava o nello stereo, oppure semplifica l'arrangiamento in quel punto.",
                                          "The overlap remains: try a sidechained dynamic EQ, move one of the two instruments by an octave or in the stereo field, or simplify the arrangement there.");
    if (k == "loud")       return tr ("Se il limiter non arriva all'obiettivo senza distorcere, lavora prima sul mix: 2 dB di compressione sul bus, sub di cassa e basso sotto controllo, poi limita in due stadi da 2-3 dB.",
                                      "If the limiter can't reach the target without distortion, work on the mix first: 2 dB of bus compression, kick and bass sub under control, then limit in two 2-3 dB stages.");
    if (k == "mixpeak")    return tr ("Attiva il controllo true peak del limiter se c'e', oppure abbassa il ceiling a -1,5 dB.",
                                      "Enable the limiter's true peak mode if available, or lower the ceiling to -1.5 dB.");
    if (k == "premaster")  return tr ("Abbassa il fader del Main o seleziona tutte le tracce e abbassale insieme; togli i processori dal bus.",
                                      "Lower the Main fader or select all tracks and lower them together; remove processors from the bus.");
    if (k.startsWith ("tone")) return tr ("Correggi traccia per traccia partendo da chi contribuisce di piu' in quella banda; sul master usa al massimo 1 dB e confronta con il riferimento.",
                                          "Fix it track by track starting from the main contributor in that band; on the master use 1 dB at most and compare with the reference.");
    if (k == "corr" || k == "lowcorr") return tr ("Trova la traccia responsabile: metti in solo le tracce stereo e ascolta in mono, quella che si svuota e' il problema.",
                                                  "Find the culprit: solo the stereo tracks and listen in mono, the one that thins out is the problem.");
    if (k == "mixsquash")  return tr ("Riduci il guadagno del limiter e la compressione sul bus; cerca l'impatto comprimendo la batteria invece del master.",
                                      "Reduce limiter gain and bus compression; get punch by compressing the drums rather than the master.");
    if (k == "mixdyn")     return tr ("Comprimi prima i gruppi (batteria, chitarre, voci) e poi il bus con 1-2 dB.",
                                      "Compress the groups first (drums, guitars, vocals), then the bus by 1-2 dB.");
    return tr ("Premi Azzera analisi dopo ogni modifica e fai suonare almeno strofa e ritornello.",
               "Press Reset analysis after every change and play at least verse and chorus.");
}

void Advisor::applyDoneMarks (juce::Array<Advice>& list, int slot, bool analysed)
{
    auto& bus = SharedBus::get();
    auto* marks = bus.marksFor (slot);
    if (marks == nullptr) return;
    const auto epochNow = bus.epochFor (slot);

    for (int i = 0; i < kMaxDone; ++i)
    {
        const auto& m = marks[i];
        if (! m.used) continue;
        const auto key = juce::String::fromUTF8 (m.key, (int) strnlen (m.key, sizeof (m.key)));
        const auto title = juce::String::fromUTF8 (m.title, (int) strnlen (m.title, sizeof (m.title)));
        const bool verified = analysed && (epochNow != m.epoch || key == "lvl");

        int found = -1;
        for (int j = 0; j < list.size(); ++j)
            if (list.getReference (j).key == key) { found = j; break; }

        if (found >= 0)
        {
            auto& a = list.getReference (found);
            if (verified)
            {
                a.status = 2;
                a.severity = juce::jmax (1, a.severity);
                if (a.fix.type == FixType::Hpf) { a.fix.freq *= 1.25f; a.fix.slope = 24; }   // secondo livello: piu' alto e piu' ripido
                a.detail = tr ("Segnato come fatto, ma il problema c'e' ancora. ", "Marked as done, but the problem is still there. ")
                           + followUp (key) + "\n" + tr ("Consiglio originale: ", "Original advice: ") + a.detail;
            }
            else
            {
                a.status = 1;
                a.detail = tr ("Fatto: premi Azzera analisi e fai suonare il brano per verificare. ", "Done: press Reset analysis and play the song to verify. ") + a.detail;
            }
        }
        else if (verified)
        {
            Advice a;
            a.key = key;
            a.status = 3;
            a.severity = 0;
            a.category = tr ("Risolto", "Solved");
            a.title = title;
            a.detail = tr ("Dopo la modifica il problema non viene piu' rilevato.", "After your change the problem is no longer detected.");
            list.add (a);
        }
    }

    // ordine: ancora presenti e importanti in alto, risolti in fondo
    std::stable_sort (list.begin(), list.end(), [] (const Advice& x, const Advice& y)
    {
        const int rx = x.status == 3 ? -1 : x.severity + (x.status == 2 ? 1 : 0);
        const int ry = y.status == 3 ? -1 : y.severity + (y.status == 2 ? 1 : 0);
        return rx > ry;
    });
}
} // namespace mpm
