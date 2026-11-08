#include "PluginCatalog.h"
#include "SharedBus.h"

namespace mpm
{
PluginCatalog& PluginCatalog::get()
{
    static PluginCatalog* instance = new PluginCatalog();   // mai distrutto (vedi retain/release)
    return *instance;
}

PluginCatalog::PluginCatalog() : juce::Thread ("MayProMix plug-in scan") {}

PluginCatalog::~PluginCatalog()
{
    stopThread (3000);
}

void PluginCatalog::retain()
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (++users == 1 && ! scanned && ! isThreadRunning())
        startThread (juce::Thread::Priority::low);
}

void PluginCatalog::release()
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (--users <= 0)
    {
        users = 0;
        stopThread (3000);
    }
}

static juce::StringArray patternsFor (Tool t);

juce::Array<juce::File> PluginCatalog::searchFolders()
{
    juce::Array<juce::File> dirs;
    auto add = [&dirs] (const juce::String& p) { if (p.isNotEmpty()) dirs.addIfNotAlreadyThere (juce::File (p)); };
    const auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getFullPathName();

   #if JUCE_WINDOWS
    auto env = [] (const char* name, const char* fallback)
    {
        return juce::SystemStats::getEnvironmentVariable (name, fallback);
    };
    const auto common = env ("CommonProgramFiles", "C:\\Program Files\\Common Files");
    const auto commonX86 = env ("CommonProgramFiles(x86)", "C:\\Program Files (x86)\\Common Files");
    const auto progs = env ("ProgramFiles", "C:\\Program Files");
    const auto local = env ("LOCALAPPDATA", "");
    add (common + "\\VST3");
    add (commonX86 + "\\VST3");
    add (common + "\\CLAP");
    add (progs + "\\VSTPlugins");
    add (progs + "\\Steinberg\\VSTPlugins");
    add (progs + "\\Common Files\\VST2");
    if (local.isNotEmpty()) add (local + "\\Programs\\Common\\VST3");
   #elif JUCE_MAC
    for (auto base : { juce::String ("/Library/Audio/Plug-Ins"), home + "/Library/Audio/Plug-Ins" })
    {
        add (base + "/VST3");
        add (base + "/Components");
        add (base + "/CLAP");
        add (base + "/VST");
    }
   #else
    add (home + "/.vst3");
    add ("/usr/lib/vst3");
    add ("/usr/local/lib/vst3");
    add (home + "/.clap");
   #endif
    return dirs;
}

void PluginCatalog::run()
{
    juce::StringArray names;
    const juce::StringArray exts { ".vst3", ".component", ".clap", ".dll", ".vst" };

    std::function<void (const juce::File&, int)> scan = [&] (const juce::File& dir, int depth)
    {
        if (threadShouldExit() || depth > 3) return;
        for (const auto& entry : juce::RangedDirectoryIterator (dir, false, "*", juce::File::findFilesAndDirectories))
        {
            const auto& f = entry.getFile();
            const auto ext = f.getFileExtension().toLowerCase();
            if (exts.contains (ext))
                names.addIfNotAlreadyThere (f.getFileNameWithoutExtension());
            else if (entry.isDirectory())
                scan (f, depth + 1);
        }
    };

    for (auto& d : searchFolders())
        if (d.isDirectory())
            scan (d, 0);

    names.removeDuplicates (true);
    names.sortNatural();

    // non suggeriamo noi stessi
    for (int i = names.size(); --i >= 0;)
        if (names[i].containsIgnoreCase ("May Pro Mix") || names[i].containsIgnoreCase ("Midi Drummer"))
            names.remove (i);

    if (threadShouldExit()) return;

    juce::StringArray lower;
    for (auto& n : names) lower.add (n.toLowerCase());

    juce::StringArray results[(int) Tool::NumTools];
    for (int t = 0; t < (int) Tool::NumTools; ++t)
        for (auto& pat : patternsFor ((Tool) t))          // l'ordine dei pattern rispecchia la priorita'
            for (int i = 0; i < names.size(); ++i)
                if (lower[i].contains (pat) && ! results[t].contains (names[i]))
                    results[t].add (names[i]);

    {
        const juce::ScopedLock sl (lock);
        installed = names;
        for (int t = 0; t < (int) Tool::NumTools; ++t) found[t] = results[t];
        waves = lower.joinIntoString ("|").contains ("waveshell");
        scanned = true;
    }
    ready = true;
}

juce::StringArray PluginCatalog::installedPlugins() const
{
    const juce::ScopedLock sl (lock);
    return installed;
}

bool PluginCatalog::hasWaves() const
{
    const juce::ScopedLock sl (lock);
    return waves;
}

static juce::StringArray patternsFor (Tool t)
{
    switch (t)
    {
        case Tool::EQ:         return { "pro-q", "neutron", "nova", "kirchhoff", "ssl", "pultec", "eqp", "maag", "api 5", "api-5", "vintage eq", "pro eq", "channel strip", "equalizer", " eq", "-eq" };
        case Tool::Compressor: return { "pro-c", "cla-76", "1176", "la-2a", "cla-2a", "kotelnikov", "rcomp", "distressor", "molot", "dbx", "vari", "glue", "bus comp", "opto", "compressor" };
        case Tool::DeEsser:    return { "pro-ds", "deesser", "de-esser", "de esser", "sibilance", "dess" };
        case Tool::Limiter:    return { "pro-l", "ozone", "elevate", "limitless", "maximizer", "limiter" };
        case Tool::Reverb:     return { "valhallaroom", "valhallavintage", "valhallaplate", "valhalla", "pro-r", "raum", "rverb", "lexicon", "seventh heaven", "blackhole", "chambers", "bricasti", "reverb", "verb" };
        case Tool::Delay:      return { "echoboy", "h-delay", "timeless", "replika", "valhalladelay", "delay", "echo" };
        case Tool::Saturation: return { "decapitator", "saturn", "radiator", "tape", "j37", "satur", "culture vulture", "drive", "tube" };
        case Tool::Imager:     return { "imager", "microshift", "stereo", "width", "panman" };
        case Tool::DynamicEQ:  return { "soothe", "pro-q", "gullfoss", "smooth operator", "resonance", "nova" };
        case Tool::Transient:  return { "transient", "trans-x", "spiff", "smack attack", "punch" };
        case Tool::Gate:       return { "pro-g", "gate" };
        case Tool::Multiband:  return { "pro-mb", "multiband", "c6", "ozone" };
        default:               return {};
    }
}

juce::StringArray PluginCatalog::foundFor (Tool t) const
{
    const int i = (int) t;
    if (i < 0 || i >= (int) Tool::NumTools) return {};
    const juce::ScopedLock sl (lock);
    return found[i];
}

juce::String PluginCatalog::nativeFor (Tool t)
{
    switch (t)
    {
        case Tool::EQ:         return "Pro EQ3";
        case Tool::Compressor: return "Compressor";
        case Tool::DeEsser:    return "De-Esser";
        case Tool::Limiter:    return "Limiter2";
        case Tool::Reverb:     return "Studio Verb";
        case Tool::Delay:      return "Analog Delay";
        case Tool::Saturation: return "Console Shaper";
        case Tool::Imager:     return "Binaural Pan";
        case Tool::DynamicEQ:  return tr ("Pro EQ3 (banda dinamica)", "Pro EQ3 (dynamic band)");
        case Tool::Transient:  return tr ("Compressor (attacco lento)", "Compressor (slow attack)");
        case Tool::Gate:       return "Gate";
        case Tool::Multiband:  return "Multiband Dynamics";
        default:               return {};
    }
}

juce::String PluginCatalog::suggest (Tool t) const
{
    auto s = nativeFor (t) + tr (" (nativo Studio Pro)", " (Studio Pro native)");
    const auto list = foundFor (t);
    if (! list.isEmpty())
    {
        juce::StringArray firstTwo;
        for (int i = 0; i < juce::jmin (2, list.size()); ++i) firstTwo.add (list[i]);
        s << tr ("  |  installati: ", "  |  installed: ") << firstTwo.joinIntoString (", ");
    }
    return s;
}
} // namespace mpm
