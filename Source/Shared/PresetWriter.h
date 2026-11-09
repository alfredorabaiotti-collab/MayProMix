#pragma once
#include <juce_core/juce_core.h>
#include "BinaryData.h"
#include "FixModel.h"

// Crea preset nativi di Fender Studio Pro (Pro EQ3 e Compressor) dai valori preparati da May Pro Mix.
// Formato verificato su preset salvati da Studio Pro 8.2: archivio ZIP con metainfo.xml e data.dsppreset (JSON).
namespace mpm::presets
{
inline juce::String num (double v)    { return juce::String (v, 4); }          // JSON: sempre il punto decimale
inline juce::String flag (bool on)    { return on ? "1.0" : "0.0"; }

inline juce::String fromBinary (const char* data, int size) { return juce::String::fromUTF8 (data, size); }

inline juce::String metainfo (const char* data, int size, const juce::String& title)
{
    auto m = fromBinary (data, size);
    m = m.replace ("@SLOTUID@", "{" + juce::Uuid().toDashedString().toUpperCase() + "}");
    m = m.replace ("@TITLE@", title.replace ("\"", "'").replace ("<", "(").replace (">", ")").replace ("&", "e"));
    return juce::String::fromUTF8 ("\xef\xbb\xbf") + m;     // Studio Pro scrive l'XML con il BOM UTF-8
}

inline bool writeZip (const juce::File& target, const juce::String& data, const juce::String& meta)
{
    juce::ZipFile::Builder zip;
    zip.addEntry (new juce::MemoryInputStream (data.toRawUTF8(), data.getNumBytesAsUTF8(), true), 6, "data.dsppreset", juce::Time::getCurrentTime());
    zip.addEntry (new juce::MemoryInputStream (meta.toRawUTF8(), meta.getNumBytesAsUTF8(), true), 6, "metainfo.xml", juce::Time::getCurrentTime());
    target.deleteFile();
    juce::FileOutputStream os (target);
    if (! os.openedOk()) return false;
    const bool ok = zip.writeToStream (os, nullptr);
    os.flush();
    return ok && ! os.getStatus().failed();
}

// Pro EQ3: passa-alto -> Low Cut, campane -> bande LMF/MF/HMF (ordinate per frequenza), shelf -> bande LF/HF
inline bool writeProEq (const FixSettings& s, const juce::File& target, const juce::String& title)
{
    auto t = fromBinary (BinaryData::proeq_template_json, BinaryData::proeq_template_jsonSize);

    t = t.replace ("@LCON@", flag (s.enabled && s.hpfOn))
         .replace ("@LCFREQ@", num (s.hpfFreq))
         .replace ("@LCSLOPE@", s.hpfSlope >= 24 ? "2.0" : "1.0");

    struct Bell { float f, g, q; };
    juce::Array<Bell> bells;
    auto active = [&s] (int k) { return s.enabled && s.eqOn[k] && std::abs (s.eqGain[k]) >= 0.05f; };
    const bool lfOn = active (0), hfOn = active (kEqBands - 1);
    const float lfF = lfOn ? s.eqFreq[0] : 110.0f, lfG = lfOn ? s.eqGain[0] : 0.0f;
    const float hfF = hfOn ? s.eqFreq[kEqBands - 1] : 11000.0f, hfG = hfOn ? s.eqGain[kEqBands - 1] : 0.0f;
    for (int k = 1; k <= 3; ++k)
        if (active (k)) bells.add ({ s.eqFreq[k], s.eqGain[k], s.eqQ[k] });
    std::sort (bells.begin(), bells.end(), [] (const Bell& a, const Bell& b) { return a.f < b.f; });
    const float defF[3] { 500.0f, 2000.0f, 5000.0f };
    for (int i = 0; i < 3; ++i)
    {
        const bool on = i < bells.size();
        const auto b = on ? bells[i] : Bell { defF[i], 0.0f, 1.0f };
        const juce::String p = "@B" + juce::String (i);
        t = t.replace (p + "ON@", flag (on)).replace (p + "FREQ@", num (b.f)).replace (p + "GAIN@", num (b.g)).replace (p + "Q@", num (b.q));
    }
    t = t.replace ("@LFON@", flag (lfOn)).replace ("@LFFREQ@", num (lfF)).replace ("@LFGAIN@", num (lfG))
         .replace ("@HFON@", flag (hfOn)).replace ("@HFFREQ@", num (hfF)).replace ("@HFGAIN@", num (hfG));

    if (t.contains ("@")) { jassertfalse; return false; }
    return writeZip (target, t, metainfo (BinaryData::proeq_metainfo_xml, BinaryData::proeq_metainfo_xmlSize, title));
}

inline bool writeCompressor (const FixSettings& s, const juce::File& target, const juce::String& title)
{
    auto t = fromBinary (BinaryData::comp_template_json, BinaryData::comp_template_jsonSize);
    t = t.replace ("@RATIO@", num (juce::jlimit (1.0f, 20.0f, s.compRatio)))
         .replace ("@THR@", num (juce::jlimit (-60.0f, 0.0f, s.compThr)))
         .replace ("@ATT@", num (juce::jlimit (0.1f, 200.0f, s.compAtt)))
         .replace ("@REL@", num (juce::jlimit (5.0f, 2000.0f, s.compRel)))
         .replace ("@GAIN@", num (juce::jlimit (0.0f, 24.0f, s.compMakeup)));
    if (t.contains ("@")) { jassertfalse; return false; }
    return writeZip (target, t, metainfo (BinaryData::comp_metainfo_xml, BinaryData::comp_metainfo_xmlSize, title));
}

// cartella temporanea per i file da trascinare
inline juce::File dragFolder()
{
    auto d = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("May Pro Mix Presets");
    d.createDirectory();
    return d;
}

inline juce::String safeName (juce::String s)
{
    s = s.removeCharacters ("\\/:*?\"<>|").trim();
    return s.isEmpty() ? juce::String ("Traccia") : s.substring (0, 48);
}
// scrive il preset del plug-in nativo (0 Pro EQ3, 1 Compressor) nella cartella temporanea; File{} se non c'e' nulla da esportare
inline juce::File writeForTrack (const FixSettings& s, int kind, const juce::String& trackName)
{
    const auto name = safeName (trackName);
    if (kind == 0 && s.hasEq())
    {
        const auto f = dragFolder().getChildFile (name + " - May Pro Mix EQ.preset");
        return writeProEq (s, f, name + " May Pro Mix") ? f : juce::File();
    }
    if (kind == 1 && s.compOn)
    {
        const auto f = dragFolder().getChildFile (name + " - May Pro Mix Comp.preset");
        return writeCompressor (s, f, name + " May Pro Mix") ? f : juce::File();
    }
    return {};
}
} // namespace mpm::presets
